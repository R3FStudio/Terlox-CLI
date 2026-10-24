'use strict';

module.exports = async function cancelSuperseded({github, context, core, wait = ms => new Promise(resolve => setTimeout(resolve, ms))}) {
    const repo = context.repo;
    const branch = context.ref.replace(/^refs\/heads\//, '');
    const stillCurrent = async () => {
        const currentHead = await github.rest.repos.getBranch({...repo, branch});
        if (currentHead.data.commit.sha === context.sha) return true;
        core.info('The branch head changed; leaving its workflow runs alone.');
        return false;
    };
    if (!await stillCurrent()) return;
    const current = await github.rest.actions.getWorkflowRun({...repo, run_id: context.runId});
    const cutoff = Date.parse(current.data.created_at);
    const runs = await github.paginate(github.rest.actions.listWorkflowRunsForRepo, {...repo, branch, per_page: 100});
    const targets = runs.filter(run =>
        run.id !== context.runId && run.head_sha !== context.sha && run.status !== 'completed' &&
        (run.event === 'push' || run.event === 'pull_request') &&
        !run.pull_requests?.some(pr => pr.head?.sha === context.sha) &&
        run.head_branch === branch && run.head_repository?.full_name?.toLowerCase() === `${repo.owner}/${repo.repo}`.toLowerCase() &&
        Date.parse(run.created_at) <= cutoff && run.id < context.runId);

    for (const run of targets) {
        if (!await stillCurrent()) return;
        try {
            await github.rest.actions.cancelWorkflowRun({...repo, run_id: run.id});
            core.info(`Requested cancellation for superseded commit ${run.head_sha}: ${run.name} (${run.id}). Keeping ${context.sha}.`);
        } catch (error) {
            // A run can finish between listing it and sending cancellation.
            if (error.status !== 409) throw error;
        }
    }
    if (!targets.length) { core.info('No older active runs on this branch.'); return; }
    await wait(10000);
    for (const run of targets) {
        if (!await stillCurrent()) return;
        const status = await github.rest.actions.getWorkflowRun({...repo, run_id: run.id});
        if (status.data.status === 'completed') continue;
        try {
            await github.request('POST /repos/{owner}/{repo}/actions/runs/{run_id}/force-cancel', {...repo, run_id: run.id});
            core.info(`Forced cancellation of ${run.name} (${run.id}).`);
        } catch (error) {
            if (error.status !== 409) throw error;
        }
    }
};
