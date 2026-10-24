'use strict';
const {test} = require('node:test');
const assert = require('node:assert/strict');
const cancel = require('../scripts/Cancel-Superseded.cjs');

function fixture(runs, {stuck = false, head = 'current', cancelError} = {}) {
    const requested = [], forced = [], waits = [];
    let branchRead = 0;
    const api = {
        repos: {getBranch: async () => ({data: {commit: {sha: Array.isArray(head)
            ? head[Math.min(branchRead++, head.length - 1)] : head}}})},
        actions: {
            listWorkflowRunsForRepo() {},
            getWorkflowRun: async ({run_id}) => ({data: run_id === 100
                ? {created_at: '2026-01-02T00:00:00Z'} : {status: stuck ? 'in_progress' : 'completed'}}),
            cancelWorkflowRun: async ({run_id}) => { requested.push(run_id); if (cancelError) throw {status: cancelError}; }
        }
    };
    return {requested, forced, waits, args: {
        github: {rest: api, paginate: async () => runs, request: async (_, {run_id}) => forced.push(run_id)},
        context: {repo: {owner: 'owner', repo: 'terminal'}, ref: 'refs/heads/main', sha: 'current', runId: 100},
        core: {info() {}}, wait: async ms => waits.push(ms)
    }};
}
const run = (id, changes = {}) => ({id, name: 'Build', event: 'push', head_sha: 'old', head_branch: 'main',
    head_repository: {full_name: 'owner/terminal'}, status: 'in_progress', created_at: '2026-01-01T00:00:00Z', ...changes});

test('cancels older active runs across workflows while protecting other runs', async () => {
    const f = fixture([run(1), run(2, {name: 'Contribution terms', status: 'queued'}),
        run(3, {status: 'completed'}), run(4, {head_sha: 'current'}), run(5, {head_branch: 'other'}),
        run(6, {head_repository: {full_name: 'fork/terminal'}}), run(101),
        run(7, {created_at: '2026-01-03T00:00:00Z'}),
        run(8, {head_sha: 'merge', pull_requests: [{head: {sha: 'current'}}]}),
        run(9, {event: 'workflow_dispatch'}), run(10, {event: 'release'})]);
    await cancel(f.args); assert.deepEqual(f.requested, [1, 2]); assert.deepEqual(f.forced, []);
});
test('force cancels runs that remain active after normal cancellation', async () => {
    const f = fixture([run(1)], {stuck: true}); await cancel(f.args);
    assert.deepEqual(f.requested, [1]); assert.deepEqual(f.waits, [10000]); assert.deepEqual(f.forced, [1]);
});
test('an outdated cancellation workflow cannot cancel a newer push', async () => {
    const f = fixture([run(1)], {head: 'newer'}); await cancel(f.args);
    assert.deepEqual(f.requested, []); assert.deepEqual(f.waits, []);
});
test('a run finishing during cancellation is harmless', async () => {
    const f = fixture([run(1)], {cancelError: 409}); await cancel(f.args);
    assert.deepEqual(f.forced, []);
});
test('permission errors are reported', async () => {
    const f = fixture([run(1)], {cancelError: 403});
    await assert.rejects(cancel(f.args), error => error.status === 403);
});

test('a branch update during run discovery prevents cancellation', async () => {
    const f = fixture([run(1)], {head: ['current', 'newer']});
    await cancel(f.args);
    assert.deepEqual(f.requested, []); assert.deepEqual(f.forced, []);
});

test('a branch update during the grace period prevents force cancellation', async () => {
    const f = fixture([run(1)], {stuck: true, head: ['current', 'current', 'newer']});
    await cancel(f.args);
    assert.deepEqual(f.requested, [1]); assert.deepEqual(f.forced, []);
});

test('each cancellation checks the branch head again', async () => {
    const f = fixture([run(1), run(2)], {head: ['current', 'current', 'newer']});
    await cancel(f.args);
    assert.deepEqual(f.requested, [1]); assert.deepEqual(f.forced, []);
});

test('manually requested packages are preserved even from older commits', async () => {
    const f = fixture([run(1, {event: 'workflow_dispatch'}), run(2, {event: 'pull_request'})]);
    await cancel(f.args);
    assert.deepEqual(f.requested, [2]);
});
