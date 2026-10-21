"""Exercise contributor sign-offs using an isolated temporary Git repository."""
from pathlib import Path
import importlib.util
import os
import subprocess
import tempfile
import unittest
from unittest.mock import patch

ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('contribution_check',ROOT/'scripts/Check-Contributions.py')
checker=importlib.util.module_from_spec(spec); spec.loader.exec_module(checker)


class ContributionTests(unittest.TestCase):
    def setUp(self):
        self.directory=tempfile.TemporaryDirectory(prefix='terlox-contribution-test-')
        self.addCleanup(self.directory.cleanup)
        self.previous=Path.cwd(); os.chdir(self.directory.name)
        self.addCleanup(os.chdir,self.previous)
        self.environment=patch.dict(os.environ,{'GIT_CONFIG_NOSYSTEM':'1','GIT_CONFIG_GLOBAL':os.devnull})
        self.environment.start(); self.addCleanup(self.environment.stop)
        self.git('init','-q')
        self.git('config','user.name','Test Contributor')
        self.git('config','user.email','contributor@example.invalid')
        self.git('config','commit.gpgsign','false')
        self.git('commit','--allow-empty','-m','base','-s')
        self.base=self.git('rev-parse','HEAD')

    def git(self,*arguments):
        return subprocess.run(['git',*arguments],check=True,capture_output=True,text=True).stdout.strip()

    def test_missing_signoff_and_amended_commit(self):
        self.git('commit','--allow-empty','-m','change')
        with self.assertRaisesRegex(SystemExit,'missing Signed-off-by'):
            checker.check(self.base,self.git('rev-parse','HEAD'))
        self.git('commit','--amend','--allow-empty','--no-edit','-s')
        checker.check(self.base,self.git('rev-parse','HEAD'))

    def test_coauthor_has_their_own_signoff(self):
        message='change\n\nCo-authored-by: Second Author <second@example.invalid>'
        self.git('commit','--allow-empty','-m',message,'-s')
        with self.assertRaisesRegex(SystemExit,'Second Author'):
            checker.check(self.base,self.git('rev-parse','HEAD'))
        message+='\nSigned-off-by: Second Author <second@example.invalid>'
        self.git('commit','--amend','--allow-empty','-m',message,'-s')
        checker.check(self.base,self.git('rev-parse','HEAD'))

    def test_only_full_commit_ids_are_accepted(self):
        with self.assertRaises(ValueError): checker.check('--all',self.base)


if __name__=='__main__': unittest.main()
