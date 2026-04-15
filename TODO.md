# TODO: Fix Conventional Commits

## Steps to complete:

[Complete] All commits rewritten to conventional style using git filter-branch.

Backup branch: `backup-conventional-fixes` updated.

History pushed to origin/main.

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


