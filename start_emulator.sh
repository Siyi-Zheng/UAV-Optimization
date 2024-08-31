#!/bin/bash

HOME_LAT=51.507394
HOME_LON=-0.171468

STEP_LAT=0.0008

AGENT_FILE="agent"
FIRMWARE_FILE="arducopter"

STL_PORT=5760
NET_PORT=15760

MAV_DIR="mav_"
OBJ_DIR="objects"

qgc_pid=0

cli=0
while getopts ":c" opt; do
    case ${opt} in
    c )
        cli=1
        ;;
    \? )
        echo "Usage: cmd [-c]"
        exit 1
        ;;
    esac
done

function start_firmwares {
    # Predefined positions for the 3 drones within 150m radius of home coordinates
    LAT_DRONE_2=$(echo "$HOME_LAT + 0.00080" | bc)    # About 88m north of home
    LON_DRONE_2=$(echo "$HOME_LON + 0.00060" | bc)    # About 49m east of home
    
    LAT_DRONE_1=$(echo "$HOME_LAT - 0.00100" | bc)    # About 111m south of home
    LON_DRONE_1=$(echo "$HOME_LON + 0.00120" | bc)    # About 98m east of home
    
    LAT_DRONE_3=$(echo "$HOME_LAT + 0.00135" | bc)    # About 150m north of home
    LON_DRONE_3=$(echo "$HOME_LON - 0.00183" | bc)    # About 150m west of home
    
    drone_positions=(
        "$LAT_DRONE_1,$LON_DRONE_1,0,0"  # Drone 1 position
        "$LAT_DRONE_2,$LON_DRONE_2,0,0"  # Drone 2 position
        "$LAT_DRONE_3,$LON_DRONE_3,0,0"  # Drone 3 position
    )
    
    # Start each drone with its predefined location
    i=0
    for name in $(ls $OBJ_DIR) ; do
        mav_num=${name#$MAV_DIR}
        mav_num=$((10#$mav_num))

        cd $OBJ_DIR/$name

        # Use the predefined positions
        ./$FIRMWARE_FILE --speedup 1 \
            --instance $mav_num \
            --param SYSID_THISMAV=$mav_num \
            --synthetic-clock \
            --home ${drone_positions[$i]} \
            --model + \
            --defaults copter.parm \
            --uartA tcp:0 &> /dev/null &
        
        cd $OLDPWD

        i=$((i + 1))  # Move to the next drone position
    done

    for name in $(ls $OBJ_DIR) ; do
        mav_num=${name#$MAV_DIR}
        mav_num=$((10#$mav_num))

        while ! nc -z localhost $((10 * mav_num + STL_PORT + 3)); do sleep 0.1; done
    done
}


function start_emulator {
    rm -rf /tmp/*.mavlink

    cd $OBJ_DIR/..
    export NS_LOG="WifiMac:WifiNetDevice:WifiPhy:YansErrorRateModel:YansWifiChannel:YansWifiPhy"

    if [ $cli -eq 1 ]; then
        xvfb-run --server-args="-screen 0 1024x768x24" ./emulator --clear-settings \
            --routing=aodv \
            --tracing=true \
            --PrintGroup=Wifi \
            --PrintAttributes=ns3::WifiPhy \
            --ChecksumEnabled=false \
            --ns3::YansWifiChannel::PropagationDelayModel=ns3::ConstantSpeedPropagationDelayModel \
            --ns3::YansWifiChannel::PropagationLossModel=ns3::FriisPropagationLossModel \
            --ns3::WifiPhy::RxGain=-10 \
            --ns3::WifiRemoteStationManager::NonUnicastMode=DsssRate1Mbps \
            --ns3::ConstantRateWifiManager::DataMode=DsssRate1Mbps \
            --ns3::ConstantRateWifiManager::ControlMode=DsssRate1Mbps &

    else
        ./emulator --clear-settings \
            --ChecksumEnabled=false \
            --ns3::YansWifiChannel::PropagationDelayModel=ns3::ConstantSpeedPropagationDelayModel \
            --ns3::YansWifiChannel::PropagationLossModel=ns3::FriisPropagationLossModel \
            --ns3::WifiPhy::RxGain=-10 \
            --ns3::WifiRemoteStationManager::NonUnicastMode=DsssRate1Mbps \
            --ns3::ConstantRateWifiManager::DataMode=DsssRate1Mbps \
            --ns3::ConstantRateWifiManager::ControlMode=DsssRate1Mbps &
    fi

    qgc_pid=$!
    cd $OLDPWD

    for name in $(ls $OBJ_DIR) ; do
        mav_num=${name#$MAV_DIR}
        mav_num=$((10#$mav_num))

        while ! nc -z localhost $((10 * mav_num + NET_PORT)); do sleep 0.1; done
    done
}

function start_agents {
    for name in $(ls $OBJ_DIR) ; do
        mav_num=${name#$MAV_DIR}
        mav_num=$((10#$mav_num))

        cd $OBJ_DIR/$name
        ./$AGENT_FILE -I $mav_num &> /dev/null &
        cd $OLDPWD
    done
}

function clean_up {
    rm -rf /tmp/*.mavlink

#    lsof -n -i4TCP:$((10 * mav_num + STL_PORT + 3)) | grep ESTABLISHED | awk '{ print $2 }' | xargs kill -SIGTERM &> /dev/null
    ps -o pid= -C $AGENT_FILE | xargs kill -SIGTERM &> /dev/null

    exit
}

trap clean_up SIGHUP SIGINT SIGTERM

source setup_emulator.sh
start_firmwares
start_emulator
start_agents

wait $qgc_pid
#echo "Press Enter to finish!"
#read

clean_up
