# SimpleBLE agent notes

Keep changes small and evidence-driven. Do not turn an issue report into an
architecture rewrite unless the failure has been reproduced and localized.

## Design context

Maintainer notes are available in `../design-docs/` beside this checkout.

The notes record context and constraints; the code, tests, and git history remain
the source of truth. If the folder is unavailable, avoid broad lifetime changes
and ask the maintainer when the intended behavior cannot be established locally.

## Releases

The release checklist in `docs/content/docs/extras.mdx` has every step with its
commands, and `docs/extras/` has each binding's packaging and publishing guide.
Committed versions always end in `-dev`; only the release-prep commit removes
it. Pushing to `main`, creating a release and deploying the documentation
publish versions that can't be replaced, so do them only when the maintainer
asks.

## Validation

SimpleDBus and SimpleBluez tests require D-Bus and are primarily exercised on
Linux. On macOS, run the checks that are available, keep Linux-only assumptions
explicit, and do not claim hardware reconnect behavior was verified without a
reproduction.
