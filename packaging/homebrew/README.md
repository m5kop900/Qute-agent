# Homebrew cask

`Casks/qute-agent.rb` installs both `.app` bundles and exposes the CLI as `qute`.

## Release steps
1. Build and sign/notarize on macOS:
   ```
   QUTE_SIGN_IDENTITY="Developer ID Application: ... (TEAMID)" \
   QUTE_NOTARY_PROFILE=qute-notary \
   QUTE_DIST_ZIP="$PWD/qute-agent-0.1.0.zip" \
   packaging/macos/sign-and-notarize.sh \
       build/src/overlay/qute.app build/src/settings/qute-settings.app
   ```
2. Create a GitHub release `v0.1.0` and upload `qute-agent-0.1.0.zip`.
3. Put the SHA-256 into the cask:
   ```
   shasum -a 256 qute-agent-0.1.0.zip
   ```
4. Check:
   ```
   brew audit --cask --online qute-agent
   brew install --cask ./qute-agent.rb
   qute --version
   ```
5. Submit to `homebrew/cask` with a pull request after the release exists and the checksum is filled in.

The cask is a template until the release URL and checksum are real; `REPLACE_WITH_SHA256...`
makes `brew audit` fail on purpose so an unreleased cask cannot be published by accident.

## Notes
- Both bundles must be signed with the same Team ID; Qt frameworks/plugins are signed by
  `packaging/macos/sign-and-notarize.sh` inside-out before the app.
- `Info-qute.plist.in` contains `NSAppleEventsUsageDescription`; the app uses `osascript` only to
  open Terminal for `/openinopencode`. The matching hardened-runtime entitlement is in
  `packaging/macos/entitlements.plist`.
- The daemon LaunchAgent is shipped inside `qute.app/Contents/Resources/io.github.qute.daemon.plist`.
