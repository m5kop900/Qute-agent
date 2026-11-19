#pragma once
#include <QObject>

// Lets the settings app restart the running overlay (daemon) so that saved settings apply.
// The overlay only reads a few settings when it starts, so a plain file reload is not enough.
class OverlayControl : public QObject {
    Q_OBJECT
public:
    explicit OverlayControl(QObject *parent = nullptr);

    // True when a daemon answers on the same IPC channel that `qute toggle` uses.
    Q_INVOKABLE bool isRunning() const;

    // `qute quit`, wait until the old daemon is gone, then start `qute --daemon`.
    // Returns false if the new daemon could not be started.
    Q_INVOKABLE bool restart();

private:
    static QString ipcName();
    static QString quteExecutable();
};
