"""Exercise transient package download failures without network access."""
import hashlib
import http.client
import importlib.util
from pathlib import Path
import unittest
from unittest.mock import patch, MagicMock
import urllib.error

spec=importlib.util.spec_from_file_location('package_tools',Path(__file__).resolve().parents[1]/'scripts/Fetch-Package-Tools.py')
tools=importlib.util.module_from_spec(spec); spec.loader.exec_module(tools)


class DownloadTests(unittest.TestCase):
    content=b'verified binary'
    checksum=hashlib.sha256(content).hexdigest()

    def response(self):
        response=MagicMock(); response.__enter__.return_value.read.return_value=self.content
        return response

    def test_gateway_timeout_retries_then_verifies(self):
        error=urllib.error.HTTPError('https://example.invalid',504,'timeout',{},None)
        with patch.object(tools.urllib.request,'urlopen',side_effect=[error,error,self.response()]) as fetch, patch.object(tools.time,'sleep') as sleep:
            self.assertEqual(tools.download_verified('https://example.invalid',self.checksum),self.content)
            self.assertEqual(fetch.call_count,3)
            self.assertEqual([c.args[0] for c in sleep.call_args_list],[1,2])

    def test_incomplete_download_is_retried(self):
        response=self.response(); response.__enter__.return_value.read.side_effect=http.client.IncompleteRead(b'partial',42)
        with patch.object(tools.urllib.request,'urlopen',side_effect=[response,self.response()]), patch.object(tools.time,'sleep'):
            self.assertEqual(tools.download_verified('https://example.invalid',self.checksum),self.content)

    def test_permanent_error_is_not_retried(self):
        error=urllib.error.HTTPError('https://example.invalid',404,'missing',{},None)
        with patch.object(tools.urllib.request,'urlopen',side_effect=error) as fetch, patch.object(tools.time,'sleep') as sleep:
            with self.assertRaises(urllib.error.HTTPError): tools.download_verified('https://example.invalid',self.checksum)
            self.assertEqual(fetch.call_count,1); sleep.assert_not_called()

    def test_checksum_mismatch_is_not_retried(self):
        with patch.object(tools.urllib.request,'urlopen',return_value=self.response()) as fetch:
            with self.assertRaisesRegex(RuntimeError,'checksum'): tools.download_verified('https://example.invalid','0'*64)
            self.assertEqual(fetch.call_count,1)

    def test_retry_budget_is_finite(self):
        with patch.object(tools.urllib.request,'urlopen',side_effect=TimeoutError()) as fetch, patch.object(tools.time,'sleep'):
            with self.assertRaises(TimeoutError): tools.download_verified('https://example.invalid',self.checksum,attempts=3)
            self.assertEqual(fetch.call_count,3)


if __name__=='__main__': unittest.main()
