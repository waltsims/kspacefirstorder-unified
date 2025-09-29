Submodule Setup

This folder contains helpers for Step 1 of the start-over plan: adding the five kspaceFirstOrder subprojects as git submodules under `repos/`.

Files
- `add_submodules.sh` — adds all five submodules and pins each to a baseline SHA.

Usage
1) Edit `add_submodules.sh` and replace `<URL>` and `<SHA>` for each entry.
   - Set `branch` if the default is not `main`.
2) Run the script from the repo root:
   - `bash scripts/submodules/add_submodules.sh`
   - Or dry-run: `DRY_RUN=1 bash scripts/submodules/add_submodules.sh`
3) Commit the changes:
   - `git add .gitmodules repos/* && git commit -m "Add kspaceFirstOrder submodules"`

Notes
- Ensure you have access to the remotes (SSH keys or HTTPS credentials as needed).
- If a target path under `repos/` already exists and is non-empty, the script will stop to avoid conflicts.
- Shallow submodules are enabled for speed; disable per-project if you need full history.

