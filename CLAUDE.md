# CLAUDE.md

**Read [`AGENTS.md`](AGENTS.md) first.** It is the authoritative contract for all agents
working in this repository, and everything in it applies to you. This file only adds what
is specific to Claude Code.

## Claude Code specifics

**CodeScene.** Use the CodeScene MCP server (`cs-mcp`): `code_health_review`,
`code_health_score` and `pre_commit_code_health_safeguard`. It is configured in
[`.mcp.json`](.mcp.json) at the repository root, which reads `CS_ACCESS_TOKEN` from the
environment; the file has a machine-specific path and is kept out of git through
`.git/info/exclude`. If the tools are missing, the server did not start: restart Claude
Code rather than working around it. **Do not call the `cs` command-line tool** for
measurement - the project owner wants the usage recorded through the MCP server.
`tools/ch.py` and `tools/codehealth_sweep.py` also talk to `cs-mcp`, for batches and
sweeps.

**Record the tool versions.** Scores move between CodeScene versions. The committed
baseline records `cs-mcp` and `cs` versions; a score you compare against it should come
from the same versions, or say that it does not.

## Refactoring campaign

Because you can reason about data flow, you are the right harness for the recipes that
need judgement: Recipe E (extract function), which must decide what crosses the
boundary, and any extraction inside an OpenMP loop body, where the playbook's rule about
which variables a block may write has to be checked by reading, not by a tool.

## Reminders that matter here

- **The oracle is cheap now - use it on every commit.** `tools/oracle/oracle.py run
  --function <Function>` runs only the cases that reach the function, usually in seconds.
  The whole catalogue (about 2 minutes warm) runs before a branch is pushed.
- **Check reach before you start, not after.** A function that no case executes is not
  protected by a green oracle run (`tools/oracle/casemap.py <Function>`). **Do not run
  mutation testing to gate a step** (owner's decision, 2026-10-27): the harness was built
  under mutation guidance and is finished. Do not read `docs/refactoring/MUTATION.md`
  whole (300 kB); search it for the function if you need the details.
- **A Code Health score of 4.00 is a milestone, not the finish line.** It marks leaving
  the Red band. Keep applying the catalogue, adding recipes as the playbook's *Growing the
  catalogue* allows, until nothing legal raises the score further; then record where the
  file plateaued and why. The target is 10.00.
- **Work from this repository only.** Do not look for recipes in other campaigns' playbooks
  or branches; new recipes come from what you find in ImageMagick.
