# Campaign kick-off

For the project owner: how to start a refactoring session on the Windows desktop. Agents
do not need to read this file; everything they follow is in `AGENTS.md`, the playbook and
the task files.

## 1. On the Windows machine: let Claude Code prepare WSL

The campaign runs inside WSL2 Ubuntu, where the harness phase ran (the oracle builds, Mull
and the case catalogue live there). You do not need to work in WSL yourself. Start Claude
Code on Windows as usual and give it this preparation prompt:

> Prepare this machine for the ImageMagick campaign; do not refactor anything. The campaign
> runs inside WSL2 Ubuntu, not in native Windows; run every command through `wsl`.
> 1. Find the WSL clone of `ImageMagick-uplift` used in the harness phase: on the Linux
>    filesystem (`~/...`, not `/mnt/c`), with a `build-oracle` directory. Tell me its path.
> 2. In that clone: `git pull` on `main`, check that `origin` is
>    `empear-analytics/ImageMagick-uplift` (fix it with `git remote set-url` if not), and run
>    `git config core.hooksPath .githooks`.
> 3. Check inside WSL that `python3`, `cs-mcp` and the `CS_ACCESS_TOKEN` variable are
>    available (do not print the token), that `python3 tools/playbook_lint.py` passes, and
>    that the `claude` command is installed. If it is not, install Claude Code inside WSL.
> 4. Check that the clone has a `.mcp.json` (machine-specific, not in git) whose CodeScene
>    entry starts the Linux `cs-mcp` with `CS_ACCESS_TOKEN`, and that
>    `python3 tools/ch.py --review MagickCore/resize.c` returns a review. Create the file if
>    it is missing. Do not use the `cs` command-line tool for any of this.
> 5. Give me one command I can paste into PowerShell that starts Claude Code inside WSL
>    in that clone (for example `wsl --cd <path> -- bash -lc claude`), and test that it
>    finds the clone.

Then close that session and start the campaign session with the command it gives you.
In it, choose Opus 5.5 with standard context (not the 1M variant), medium effort, and run
`/mcp`: the CodeScene server must be listed as connected before you give the first prompt.

Why not native Windows: the oracle, Mull and the build scripts need Linux, and files edited
from the Windows side of a WSL clone risk CRLF line endings that the guard and the oracle
would see as changes.

## 2. Prompt for the first session (task M01)

> You are starting the ImageMagick Code Health campaign. Read `AGENTS.md`, then
> `docs/refactoring/tasks/M01-resize.md` and `docs/refactoring/PLAYBOOK.md`, and work task
> M01 on a new branch `campaign/M01-resize` from `main`.
>
> This machine is WSL2, not the macOS reference machine on which the task file was
> measured. Before the first refactoring step: build, run
> `python3 tools/oracle/oracle.py selfcheck` (it must report 0 nondeterministic), and measure
> `MagickCore/resize.c` through the CodeScene MCP server. If the score is not 1.39, stop and
> report the score and the `cs-mcp` version, as the task file says.
>
> Measure Code Health only through the CodeScene MCP server: its tools, or `tools/ch.py`,
> which talks to the same server. Never use the `cs` command-line tool. If the server is
> unavailable, stop and report; do not work around it.
>
> The catalogue starts with five recipes. When findings recur that no recipe reaches, you may
> add a recipe, or change one of the three rules, as the playbook's *Growing the catalogue*
> describes: in its own commit, with a *Catalogue changes* row, and with a code and a name you
> choose. Every refactor commit carries the `Recipe:` and `Verified:` trailers; never use
> `--no-verify`.
>
> Stop when M01's exit target is reached or the file plateaus. Write the report the task file
> asks for, run the whole oracle catalogue, push the branch, and open a pull request against
> `empear-analytics/ImageMagick-uplift`. Do not merge it.

## 3. After each session

```bash
python3 tools/campaign_cost.py --by branch      # in the WSL session (or ask Claude Code to run it)
python3 tools/check_trailers.py --range main..campaign/M01-resize
python3 tools/playbook_lint.py
```

Review and merge the pull request yourself, then start the next task file in a **fresh**
session (M02 `compare.c`, ...), with the same prompt and its task number.
