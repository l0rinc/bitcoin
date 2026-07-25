# Working agreements (standing user instructions)

These rules come directly from the user and apply to every session and
agent working in this repository (and its sibling repos /mnt/my_storage/bitcoin
and /mnt/my_storage/secp256k1):

- **Commit every finding to a named branch.** Chat output is ephemeral and
  the user does not always see it. No finding, fix, triage ledger, test, or
  reproduction is "done" until it is committed somewhere durable.
- **Never leave work only on a detached HEAD.** If commits are made while
  detached, immediately create a branch pointing at them.
- Local commits on new/staging branches and docs/finding commits on the
  user's findings branches are pre-authorized. Still ask before:
  - destructive operations (reset --hard, force-push, deleting branches or
    worktrees that contain uncommitted work),
  - outward-facing operations (push, opening/commenting on PRs or issues).
- Report findings with emoji severity markers so critical items stand out:
  🔴 critical, 🟠 high, 🟡 medium/latent, ✅ verified-clean.
- Keep /tmp for scratch only (worktrees, build dirs). Anything valuable must
  land in the repository; /tmp can vanish on reboot.
