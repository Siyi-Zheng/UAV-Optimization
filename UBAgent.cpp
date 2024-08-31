#include "UBAgent.h"
#include "UBNetwork.h"

#include "UBConfig.h"

#include <QTimer>
#include <QCommandLineParser>

#include "Vehicle.h"
#include "TCPLink.h"
#include "QGCApplication.h"

#include <fstream>
#include <cmath>


UBAgent::UBAgent(QObject *parent) : QObject(parent),
    m_mav(nullptr)
{
    m_net = new UBNetwork;
    connect(m_net, SIGNAL(dataReady(quint8, QByteArray)), this, SLOT(dataReadyEvent(quint8, QByteArray)));

    m_timer = new QTimer;
    connect(m_timer, SIGNAL(timeout()), this, SLOT(missionTracker()));

    startAgent();
}

void UBAgent::startAgent() {
    QCommandLineParser parser;
    parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);

    parser.addOptions({
        {{"I", "instance"}, "Set instance (ID) of the agent", "id"},
    });

//    parser.process(*QCoreApplication::instance());
    parser.parse(QCoreApplication::arguments());

    quint8 id = parser.value("I").toUInt();
    LinkConfiguration* link = nullptr;
    if (id) {
        quint32 port = 10 * id + STL_PORT + 3;
        TCPConfiguration* tcp = new TCPConfiguration(tr("TCP Port %1").arg(port));
        tcp->setAddress(QHostAddress::LocalHost);
        tcp->setPort(port);

        link = tcp;
    } else {
        SerialConfiguration* serial = new SerialConfiguration("Serial Port");
        serial->setBaud(BAUD_RATE);
        serial->setPortName(SERIAL_PORT);

        link = serial;
    }

    link->setDynamic();
    link->setAutoConnect();

    LinkManager* linkManager = qgcApp()->toolbox()->linkManager();
    linkManager->addConfiguration(link);
    linkManager->linkConfigurationsChanged();

    connect(qgcApp()->toolbox()->multiVehicleManager(), SIGNAL(vehicleAdded(Vehicle*)), this, SLOT(vehicleAddedEvent(Vehicle*)));
    connect(qgcApp()->toolbox()->multiVehicleManager(), SIGNAL(vehicleRemoved(Vehicle*)), this, SLOT(vehicleRemovedEvent(Vehicle*)));

    m_net->connectToHost(QHostAddress::LocalHost, 10 * id + NET_PORT);
    m_timer->start(MISSION_TRACK_RATE);
}

void UBAgent::setMAV(Vehicle* mav) {
    if (m_mav) {
        disconnect(m_mav, SIGNAL(armedChanged(bool)), this, SLOT(armedChangedEvent(bool)));
        disconnect(m_mav, SIGNAL(flightModeChanged(QString)), this, SLOT(flightModeChangedEvent(QString)));
    }

    m_mav = mav;

    if (m_mav) {
        connect(m_mav, SIGNAL(armedChanged(bool)), this, SLOT(armedChangedEvent(bool)));
        connect(m_mav, SIGNAL(flightModeChanged(QString)), this, SLOT(flightModeChangedEvent(QString)));
    }
}

void UBAgent::vehicleAddedEvent(Vehicle* mav) {
    if (!mav || m_mav == mav) {
        return;
    }

    setMAV(mav);
    m_net->setID(mav->id());

    qInfo() << "New MAV connected with ID: " << m_mav->id();

    writeToFile("New MAV connected with ID: " + std::to_string(m_mav->id()));
    writeToFile("Flight Mode: " + m_mav->flightMode().toStdString());

    // Check if flight mode can be changed
    if (!m_mav->flightModeSetAvailable()) {
        writeToFile("Flight mode setting is not available.");
        return;
    }

    // Attempt to set the drone to Guided mode
    m_mav->setFlightMode("Guided");

    // Optionally, connect to flight mode changed signal to confirm mode change
    connect(m_mav, &Vehicle::flightModeChanged, this, [this](const QString& mode) {
        writeToFile("Flight mode changed to: " + mode.toStdString());
        if (mode == "Guided") {
            writeToFile("Drone is now in Guided mode.");
        }
    });

}

void UBAgent::vehicleRemovedEvent(Vehicle* mav) {
    if (!mav || m_mav != mav) {
        return;
    }

    setMAV(nullptr);
    m_net->setID(0);

    qInfo() << "MAV disconnected with ID: " << mav->id();
    writeToFile("MAV disconnected with ID: " + std::to_string(m_mav->id()));
}

void UBAgent::armedChangedEvent(bool armed) {
    if (!armed) {
        m_mission_stage = STAGE_IDLE;
        return;
    }

    if (m_mav->altitudeRelative()->rawValue().toDouble() > POINT_ZONE) {
        qWarning() << "The mission can not start while the drone is airborne!";
        return;
    }

    if (!m_mav->guidedMode()) {
        qWarning() << "The mission can not start while the drone is not in Guided mode!";
        m_mav->setFlightMode("Guided");
        return;
    }

    m_mission_data.reset();
    m_mission_stage = STAGE_TAKEOFF;
    qInfo() << "Mission starts...";
    writeToFile("Mission starts...");

//    m_mav->guidedModeTakeoff();
    m_mav->sendMavCommand(m_mav->defaultComponentId(),
                            MAV_CMD_NAV_TAKEOFF,
                            true, // show error
                            0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                            TAKEOFF_ALT);
}

void UBAgent::flightModeChangedEvent(QString mode) {
    qInfo() << mode;
}

void UBAgent::dataReadyEvent(quint8 srcID, QByteArray data) {
    Q_UNUSED(data)
    if(srcID == m_mav->id() - 1 && !m_mav->armed()) {
        m_mav->setArmed(true);
    }
}

void UBAgent::missionTracker() {

    double timeStep = MISSION_TRACK_RATE / 1000.0;  // Convert to seconds

    // Call the energy consumption function for every tick
    calculateEnergyAndUpdateBattery(timeStep);

    switch (m_mission_stage) {
    case STAGE_IDLE:
        stageIdle();
        break;
    case STAGE_TAKEOFF:
        stageTakeoff();
        break;
    case STAGE_MISSION:
        stageMission();
        break;
    case STAGE_LAND:
        stageLand();
        break;
    default:
        break;
    }
}

void UBAgent::stageIdle() {
}

void UBAgent::stageTakeoff() {
    if (m_mav->altitudeRelative()->rawValue().toDouble() > TAKEOFF_ALT - POINT_ZONE) {
        m_mission_data.stage = 0;
        m_mission_stage = STAGE_MISSION;
    }
}

void UBAgent::stageLand() {
    if (m_mav->altitudeRelative()->rawValue().toDouble() < POINT_ZONE) {
        m_mission_stage = STAGE_IDLE;
        qInfo() << "Mission ends";
    }
}

void UBAgent::stageMission(QGeoCoordinate intermediate1, QGeoCoordinate intermediate2) {
    static QGeoCoordinate dest;
    static QGeoCoordinate waypoint1;
    static QGeoCoordinate waypoint2;

    if (m_mission_data.stage == 0) {
        // Set waypoints
        waypoint1 = intermediate1;
        waypoint2 = intermediate2;
        dest = m_mav->coordinate().atDistanceAndAzimuth(500, 335);

        // First waypoint (intermediate1)
        m_mission_data.stage++;
        
        m_mav->guidedModeGotoLocation(waypoint1);
        return;
    }

    if (m_mission_data.stage == 1) {
        // Check if the drone has reached the first waypoint (intermediate1)
        if (m_mav->coordinate().distanceTo(waypoint1) < POINT_ZONE) {
            m_mission_data.stage++;
            
            m_mav->guidedModeGotoLocation(waypoint2);  // Move to the second waypoint
        }
        return;
    }

    if (m_mission_data.stage == 2) {
        // Check if the drone has reached the second waypoint (intermediate2)
        if (m_mav->coordinate().distanceTo(waypoint2) < POINT_ZONE) {
            m_mission_data.stage++;

            // Set final destination
            
            m_mav->guidedModeGotoLocation(dest);  // Move to the final destination
        }
        return;
    }

    if (m_mission_data.stage == 3) {
        // Check if the drone has reached the final destination
        if (m_mav->coordinate().distanceTo(dest) < POINT_ZONE) {
            m_mission_data.stage++;
        }
        return;
    }

    if (m_mission_data.stage == 4) {
        // Perform the task or wait for a specific time
        if (m_mission_data.tick < (5 * 1000 / MISSION_TRACK_RATE)) {
            m_mission_data.tick++;
            m_net->sendData(m_mav->id() + 1, QByteArray(1, MAV_CMD_NAV_TAKEOFF));
        } else {
            // Land after completing the mission
            m_mav->guidedModeLand();
            m_mission_stage = STAGE_LAND;
        }
    }
}

// Emergency landing procedure if battery is too low
void UBAgent::emergencyLanding() {
    if (m_mav) {
        m_mav->guidedModeLand();
        m_mission_stage = STAGE_LAND;
    }
}


void UBAgent::writeToFile(const std::string& message) {
    std::ofstream file("/home/anc-install/mission_log.txt", std::ios_base::app);  // Open the file in append mode
    if (!file.is_open()) {
        qWarning() << "Failed to open the file!";
        return;
    }

    file << message << std::endl;  // Write the message and add a new line
    file.close();  // Close the file
}

// Constants for energy consumption model
constexpr double C_EFF = 1.2e-9;   // Effective switched capacitance in farads
constexpr double CLOCK_FREQ = 1000; // Clock frequency in Hz

// Function to calculate and update energy consumption based on the UAV's computation
void UBAgent::calculateEnergyAndUpdateBattery(double timeStep) {
    // Calculate energy consumed in joules
    double energyConsumed = C_EFF * pow(CLOCK_FREQ, 2) * timeStep;

    // Convert energy consumed to battery percentage.
    // Assume that total battery energy corresponds to 100% (1.0) and that m_mav provides the battery as a percentage.
    double totalBatteryEnergy = m_mav->batteryCapacity();  // Assume batteryCapacity returns the battery energy in joules
    double batteryPercentageConsumed = (energyConsumed / totalBatteryEnergy) * 100;

    // Get current battery percentage
    double remainingBattery = m_mav->remainingBattery();

    // Update remaining battery percentage
    remainingBattery -= batteryPercentageConsumed;

    // Ensure battery percentage doesn't go below 0
    if (remainingBattery < 0) {
        remainingBattery = 0;
    }

    // Set the updated battery value back to the UAV
    m_mav->setRemainingBattery(remainingBattery);

    // Log the battery change for debugging purposes
    writeToFile("Energy consumed: " + std::to_string(energyConsumed) + " joules");
    writeToFile("Battery percentage consumed: " + std::to_string(batteryPercentageConsumed) + "%");
    writeToFile("Remaining battery: " + std::to_string(remainingBattery) + "%");

    // Check if battery is critically low and initiate emergency landing if necessary
    if (remainingBattery <= 5.0) {  // Assuming 5% as the critical battery threshold
        writeToFile("Battery critical! Initiating emergency landing...");
        emergencyLanding();
    }
}
