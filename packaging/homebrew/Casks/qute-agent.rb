cask "qute-agent" do
  version "0.1.0"
  # Replace after uploading the notarized zip:
  #   shasum -a 256 qute-agent-0.1.0.zip
  sha256 "REPLACE_WITH_SHA256_OF_qute-agent_TARBALL"

  url "https://github.com/m5kop900/Qute-agent/releases/download/v#{version}/qute-agent-#{version}.zip"
  name "Qute Agent"
  desc "Animated desktop companion for opencode"
  homepage "https://github.com/m5kop900/Qute-agent"

  depends_on macos: ">= :monterey"

  app "qute.app"
  app "qute-settings.app"
  # The CLI and the overlay are the same binary; expose it as `qute`.
  binary "#{appdir}/qute.app/Contents/MacOS/qute", target: "qute"

  caveats <<~EOS
    Qute Agent has no built-in global shortcut. Bind `qute toggle` in the
    Shortcuts app (Run Shell Script), skhd, Hammerspoon or Raycast.

    To start the daemon at login, copy the LaunchAgent:
      #{staged_path}/qute.app/Contents/Resources/io.github.qute.daemon.plist
      to ~/Library/LaunchAgents/ and adjust the path, or run `qute --daemon` from a login item.
  EOS

  zap trash: [
    "~/Library/Preferences/qute",
    "~/Library/Saved Application State/io.github.qute.overlay.savedState",
    "~/Library/Saved Application State/io.github.qute.settings.savedState",
  ]
end
