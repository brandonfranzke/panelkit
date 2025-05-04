#!/bin/bash
# Instructions for running PanelKit on the embedded target with the rendering abstraction

# The command to run on the target device:
# PANELKIT_USE_RENDERING=1 RUST_LOG=debug ./panelkit

echo "To run PanelKit on the embedded target with rendering abstraction enabled:"
echo "1. Connect to your target device:"
echo "   ssh pi@raspberrypi.local"
echo "2. Navigate to the PanelKit directory:"
echo "   cd /home/pi/panelkit"
echo "3. Run the application with rendering abstraction enabled:"
echo "   PANELKIT_USE_RENDERING=1 RUST_LOG=debug ./panelkit"
echo ""
echo "Or, you can use the following one-liner:"
echo "ssh pi@raspberrypi.local \"cd /home/pi/panelkit && PANELKIT_USE_RENDERING=1 RUST_LOG=debug ./panelkit\""