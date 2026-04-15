# TODO: Fix Conventional Commits

## Steps to complete:

[Complete] Backup created: `backup-conventional-fixes` branch created.

1. **Run interactive rebase** (recommended): Copy-paste this into your editor when running `git rebase -i --root`:

```
pick 7057299 chore: initial project setup
pick 9c6bf35 feat: add SUNDIALS submodule
pick 494ce69 feat: add initial project files
pick cdb23c4 build: add Makefile for NeoSUNDIALS C builds
pick 7330040 docs: update README
pick 87b1b58 refactor: flatten NeoSUNDIALS project layout
pick 4b04b8d feat: expand extracted solvers and tests
pick 5152209 refactor(tests): unify project test runners
pick 10269ce feat(arkode): expand extracted ARK method catalog
pick 9c2e4ff feat(nvector): add self-contained serial N_Vector layer
pick de2411f chore(tests): port SUNDIALS NVECTOR_SERIAL unit test
```

Save/quit (:wq). Edit body if prompted.

2. **Force-push**: `git push --force-with-lease origin main`

3. **Verify**: `git log --oneline -11`

4. **Optional commitlint**: See below.

**Progress**: 1/4 steps complete (backup done). No emojis used.

**Notes**: If rebase conflicts arise, `git rebase --abort` and restore from backup: `git reset --hard backup-conventional-fixes`.

## Optional: Automated commitlint setup (run after rebase)
```
npm init -y
npm i -D @commitlint/config-conventional @commitlint/cli
echo "module.exports = { extends: ['@commitlint/config-conventional'] };" > commitlint.config.js
echo "#!/usr/bin/env sh
. \"\$(dirname -- \"\$0\")/_/husky.sh\"

npx --no -- commitlint --edit \${1}" > .git/hooks/commit-msg
chmod +x .git/hooks/commit-msg
```


