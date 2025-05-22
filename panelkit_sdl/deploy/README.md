# PanelKit SDL Deployment

## Quick Deployment

Use the automated deployment script:
```bash
./deploy.sh <TARGET_IP> [user]
```

Examples:
```bash
./deploy.sh 192.168.1.100           # Deploy to pi@192.168.1.100
./deploy.sh 192.168.1.100 brandon   # Deploy to brandon@192.168.1.100
```

## Manual Installation

If you prefer manual deployment:

1. Copy the binary from build directory:
   ```bash
   scp ../build/arm64/panelkit pi@<TARGET_IP>:/tmp/
   ```

2. Install the binary on the target:
   ```bash
   ssh pi@<TARGET_IP> "sudo mv /tmp/panelkit /usr/local/bin/ && sudo chmod +x /usr/local/bin/panelkit"
   ```

3. Copy and enable the systemd service:
   ```bash
   scp panelkit.service pi@<TARGET_IP>:/tmp/
   ssh pi@<TARGET_IP> "sudo mv /tmp/panelkit.service /etc/systemd/system/ && sudo systemctl daemon-reload && sudo systemctl enable panelkit.service"
   ```

4. Start the service:
   ```bash
   ssh pi@<TARGET_IP> "sudo systemctl start panelkit.service"
   ```

## Troubleshooting

- Check service status:
  ```
  systemctl status panelkit.service
  ```

- View logs:
  ```
  journalctl -u panelkit.service
  ```

- For framebuffer access issues, ensure the user has the right permissions:
  ```
  sudo usermod -a -G video brandon
  sudo chmod 666 /dev/fb0
  ```
