#!/bin/bash
# Instructions for running PanelKit on the embedded target

# The command to run on the target device:
# RUST_LOG=debug ./panelkit

echo "To run PanelKit on the embedded target:"
echo "1. Connect to your target device:"
echo "   ssh pi@raspberrypi.local"
echo "2. Navigate to the PanelKit directory:"
echo "   cd /home/pi/panelkit"
echo "3. Run the application:"
echo "   RUST_LOG=debug ./panelkit"
echo ""
echo "Or, you can use the following one-liner:"
echo "ssh pi@raspberrypi.local \"cd /home/pi/panelkit && RUST_LOG=debug ./panelkit\""