#!/bin/bash

# Path to the log file
LOG_FILE="/home/anc-install/mission_log.txt"

# Check if the log file exists
if [ -e "$LOG_FILE" ]; then
    # Empty the log file
    > "$LOG_FILE" || { echo "Failed to clear the log file."; exit 1; }
    echo "Log file has been emptied."
else
    # Create the log file if it does not exist
    touch "$LOG_FILE" || { echo "Failed to create the log file."; exit 1; }
    echo "Log file did not exist and has been created."
fi

# Set the working directory to the UB-ANC-Agent build directory
cd ~/ub-anc/build-agent || { echo "Directory ~/ub-anc/build-agent does not exist."; exit 1; }

# Run qmake to generate Makefiles
qmake ../UB-ANC-Agent/ || { echo "qmake failed."; exit 1; }

# Compile the project
make || { echo "make failed."; exit 1; }

# Copy the compiled agent to the target directory
cp ~/ub-anc/build-agent/agent/release/agent ~/ub-anc/emulator/mav/agent || { echo "cp failed."; exit 1; }

# Change to the emulator directory
cd ~/ub-anc/emulator || { echo "Directory ~/ub-anc/emulator does not exist."; exit 1; }

# Set up the objects
./setup_objects.sh 3 || { echo "setup_objects.sh failed."; exit 1; }

# Start the emulator
./start_emulator.sh || { echo "start_emulator.sh failed."; exit 1; }

echo "Build and deployment completed successfully."

