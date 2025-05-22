# PanelKit SDL Deployment

## Installation

1. Copy the 'panelkit' binary to the target device:
   ```
   scp panelkit pi@<TARGET_IP>:/tmp/
   ```

2. Install the binary on the target:
   ```
   ssh pi@<TARGET_IP> "sudo mv /tmp/panelkit /usr/local/bin/ && sudo chmod +x /usr/local/bin/panelkit"
   ```

3. Copy and enable the systemd service:
   ```
   scp panelkit.service pi@<TARGET_IP>:/tmp/
   ssh pi@<TARGET_IP> "sudo mv /tmp/panelkit.service /etc/systemd/system/ && sudo systemctl daemon-reload && sudo systemctl enable panelkit.service"
   ```

4. Start the service:
   ```
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
