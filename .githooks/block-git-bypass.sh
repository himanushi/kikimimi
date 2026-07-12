#!/usr/bin/env bash
# Claude Code PreToolUse hook: git hook のバイパスを禁止する
# (--no-verify / --no-gpg-sign / core.hooksPath の付け替え)
cmd=$(jq -r '.tool_input.command // ""' 2>/dev/null)

if printf '%s' "$cmd" | grep -qE -- '--no-verify|--no-gpg-sign'; then
  echo "禁止: git hook をバイパスするフラグ(--no-verify / --no-gpg-sign)は使用できません。pre-commit が失敗する場合は原因を修正してください" >&2
  exit 2
fi

if printf '%s' "$cmd" | grep -q 'core\.hooksPath' && ! printf '%s' "$cmd" | grep -q 'core\.hooksPath .githooks'; then
  echo "禁止: core.hooksPath の変更は .githooks 以外に設定できません" >&2
  exit 2
fi

exit 0
