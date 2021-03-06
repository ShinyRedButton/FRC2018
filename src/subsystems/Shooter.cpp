#include "RobotInfo.h"
#include "lib/util/Util.h"
#include "lib/WrapDash.h"
#include "lib/TaskMgr.h"
#include "lib/logging/LogSpreadsheet.h"
#include "subsystems/Shooter.h"

namespace frc973 {

Shooter::Shooter(TaskMgr *scheduler, LogSpreadsheet *logger,
            CANTalon *leftAgitator, Drive *drive, BoilerPixy *boilerPixy) :
        m_scheduler(scheduler),
        m_flywheelState(FlywheelState::notRunning),
        m_shootingSequenceState(ShootingSequenceState::idle),
        m_side(Side::left),
        m_flywheelMotorPrimary(new CANTalon(FLYWHEEL_PRIMARY_CAN_ID,
                                            FLYWHEEL_CONTROL_PERIOD_MS)),
        m_flywheelMotorReplica(new CANTalon(FLYWHEEL_REPLICA_CAN_ID)),
        m_kicker(new CANTalon(KICKER_CAN_ID)),
        m_leftAgitator(leftAgitator),
        m_rightAgitator(new CANTalon(RIGHT_AGITATOR_CAN_ID, 50)),
        m_ballConveyor(new CANTalon(BALL_CONVEYOR_CAN_ID, 50)),
        m_flywheelPow(0.0),
        m_flywheelSpeedSetpt(0.0),
        m_kickerSpeedSetpt(0.0),
        m_flywheelOnTargetFilter(0.2),
        m_drive(drive),
        m_boilerPixy(boilerPixy)
{
    m_flywheelMotorPrimary->SetFeedbackDevice(CANTalon::FeedbackDevice::CtreMagEncoder_Relative);
    m_flywheelMotorPrimary->ConfigNeutralMode(CANSpeedController::NeutralMode::kNeutralMode_Coast);
    m_flywheelMotorPrimary->SetClosedLoopOutputDirection(false);
    m_flywheelMotorPrimary->SetNominalClosedLoopVoltage(12.0);
    m_flywheelMotorPrimary->ConfigLimitSwitchOverrides(false, false);
    m_flywheelMotorPrimary->SetSensorDirection(false);
    m_flywheelMotorPrimary->SetControlMode(CANSpeedController::ControlMode::kSpeed);
    m_flywheelMotorPrimary->SelectProfileSlot(0);
    m_flywheelMotorPrimary->ConfigNominalOutputVoltage(0, 0);
    m_flywheelMotorPrimary->ConfigPeakOutputVoltage(12, 0.0);
    m_flywheelMotorPrimary->SetP(0.32);
    m_flywheelMotorPrimary->SetI(0.00004);
    m_flywheelMotorPrimary->SetD(0.00);
    m_flywheelMotorPrimary->SetF(0.022);
    m_flywheelMotorPrimary->SetIzone(1000);
    m_flywheelMotorPrimary->SetVelocityMeasurementPeriod(CANTalon::Period_10Ms);
    m_flywheelMotorPrimary->SetVelocityMeasurementWindow(32);

    m_flywheelMotorReplica->ConfigNeutralMode(
            CANSpeedController::NeutralMode::kNeutralMode_Coast);
    m_flywheelMotorReplica->SetControlMode(
            CANSpeedController::ControlMode::kFollower);
    m_flywheelMotorReplica->Set(m_flywheelMotorPrimary->GetDeviceID());
    m_flywheelMotorReplica->SetClosedLoopOutputDirection(true);
    m_flywheelMotorReplica->SetNominalClosedLoopVoltage(12.0);

    m_kicker->SetFeedbackDevice(CANTalon::FeedbackDevice::CtreMagEncoder_Relative);
    m_kicker->ConfigNeutralMode(CANSpeedController::NeutralMode::kNeutralMode_Coast);
    m_kicker->SetClosedLoopOutputDirection(false);
    m_kicker->SetNominalClosedLoopVoltage(12.0);
    m_kicker->ConfigLimitSwitchOverrides(false, false);
    m_kicker->SetSensorDirection(false);
    m_kicker->SetControlMode(CANSpeedController::ControlMode::kSpeed);
    m_kicker->SelectProfileSlot(0);
    m_kicker->ConfigNominalOutputVoltage(0, 0);
    m_kicker->ConfigPeakOutputVoltage(12, 0.0);
    m_kicker->SetP(0.1);
    m_kicker->SetI(0.0);
    m_kicker->SetD(0.0);
    m_kicker->SetF(0.018);
    m_kicker->SetIzone(1000);
    m_kicker->SetVelocityMeasurementPeriod(CANTalon::Period_10Ms);
    m_kicker->SetVelocityMeasurementWindow(32);

    m_leftAgitator->SetControlMode(CANSpeedController::ControlMode::kVoltage);
    m_rightAgitator->SetControlMode(CANSpeedController::ControlMode::kVoltage);
    m_ballConveyor->SetControlMode(CANSpeedController::ControlMode::kVoltage);

    m_leftAgitator->EnableCurrentLimit(true);
    m_rightAgitator->EnableCurrentLimit(true);
    m_ballConveyor->EnableCurrentLimit(true);
    m_leftAgitator->SetCurrentLimit(40);
    m_rightAgitator->SetCurrentLimit(40);
    m_ballConveyor->SetCurrentLimit(40);
    m_leftAgitator->SetVoltageRampRate(120.0);
    m_rightAgitator->SetVoltageRampRate(120.0);
    m_ballConveyor->SetVoltageRampRate(120.0);

    m_scheduler->RegisterTask("Shooter", this, TASK_PERIODIC);
    m_flywheelRate = new LogCell("FlywheelRate", 32);
    m_flywheelPowLog = new LogCell("Flywheel voltage", 32);
    m_flywheelAmpsLog = new LogCell("Flywheel current", 32);
    m_flywheelStateLog = new LogCell("FlywheelState", 32);
    m_speedSetpoint = new LogCell("SpeedSetpoint", 32);
    m_conveyorLog = new LogCell("Conveyor curr", 32);
    m_leftAgitatorLog = new LogCell("leftAgitator curr", 32);
    m_rightAgitatorLog = new LogCell("RightAgitator curr", 32);
    logger->RegisterCell(m_flywheelRate);
    logger->RegisterCell(m_flywheelPowLog);
    logger->RegisterCell(m_flywheelAmpsLog);
    logger->RegisterCell(m_flywheelStateLog);
    logger->RegisterCell(m_speedSetpoint);
    logger->RegisterCell(m_conveyorLog);
    logger->RegisterCell(m_leftAgitatorLog);
    logger->RegisterCell(m_rightAgitatorLog);
}

Shooter::~Shooter() {
    m_scheduler->UnregisterTask(this);
}

/**
 * Sets flywheel on with open loop
 *
 * @param pow the amount of flywheel power from 1.0 to -1.0
 */
void Shooter::SetFlywheelPow(double pow){
    m_flywheelMotorPrimary->SetControlMode(CANSpeedController::ControlMode::kPercentVbus);
    m_flywheelState = FlywheelState::power;
    m_flywheelPow = pow;
}

/**
 * Sets flywheel on with closed loop
 *
 * @param pow the flywheel speed; preferably less than 5000 RPM
 */
void Shooter::SetFlywheelSpeed(double speed){
    m_flywheelMotorPrimary->SetControlMode(CANSpeedController::ControlMode::kSpeed);
    m_flywheelState = FlywheelState::speed;
    m_flywheelSpeedSetpt = speed;
    m_flywheelMotorPrimary->Set(m_flywheelSpeedSetpt);
    m_kicker->Set(3000);
    m_kickerSpeedSetpt = 3000;
}

void Shooter::SetFlywheelStop(){
    m_flywheelPow = 0.0;
    m_flywheelMotorPrimary->SetControlMode(CANSpeedController::ControlMode::kPercentVbus);
    m_flywheelMotorPrimary->Set(0.0);
    m_flywheelState = FlywheelState::notRunning;
    m_flywheelOnTargetFilter.Update(false);
    m_kicker->Set(0.0);
    m_kickerSpeedSetpt = 0.0;
}

/**
 * Returns flywheel rate through encoder translation
 *
 * @return the flywheel rate
 */
double Shooter::GetFlywheelRate(){
    return m_flywheelMotorPrimary->GetSpeed();// * (1.0 / 24576.0);
}

/**
 * Returns if flywheel rate is in the 200 range or up to speed
 *
 * @return tif flywheel rate is up to speed
 */
bool Shooter::OnTarget() {
    return m_flywheelOnTargetFilter.Update(
            abs(GetFlywheelRate() - m_flywheelSpeedSetpt) < 200.0);
}

/**
 * Sets conveyor power in open loop
 *
 * @param speed desired conveyor speed from -1.0 to 1.0
 */
void Shooter::StartConveyor(double speed) {
    m_ballConveyor->Set(speed * 12.0);

    //printf("%lf pow on %d - conveyor\n", speed, BALL_CONVEYOR_CAN_ID);
    //DBStringPrintf(DB_LINE3, "conv pow %lf", speed);
}

void Shooter::StopConveyor() {
    m_ballConveyor->Set(0.0);
    //printf("%lf pow on %d - conveyor\n", 0.0, BALL_CONVEYOR_CAN_ID);
    //DBStringPrintf(DB_LINE3, "conv pow %lf", 0.0);
}

/**
 * Starts agitator by side (needs to call method twice in order to turn on each agitator)
 *
 * @param speed  desired agitator speed from -1.0 to 1.0
 * @param side   desired side
 */
void Shooter::StartAgitator(double speed, Side side){
    if (side == Side::left) {
        m_leftAgitator->Set(speed * 12.0);
        //printf("%lf pow on %d - left agitator\n", speed, LEFT_AGITATOR_CAN_ID);
    }
    else if (side == Side::right) {
        m_rightAgitator->Set(-speed * 12.0);
        //printf("%lf pow on %d - right agitator\n", speed, RIGHT_AGITATOR_CAN_ID);
    }
}

/**
 * Sets shooter state either manual, idle, or shooting
 *
 * @param state  desired shooting state
 */
void Shooter::SetShooterState(ShootingSequenceState state){
  m_shootingSequenceState = state;
  //m_flywheelOnTargetFilter.Update(false);
}

void Shooter::StopAgitator(){
    m_leftAgitator->Set(0.0);
    m_rightAgitator->Set(0.0);
}

/**
 * Sets kicker rate in closed loop
 *
 * @param speed desired kicker speed (preferably close to main shooter RPM)
 */
void Shooter::SetKickerRate(double speed){
  m_kicker->Set(speed);
  m_kickerSpeedSetpt = speed;
}

/**
 * Return kicker rate through encoder translation
 *
 * @return kicker speed
 */
double Shooter::GetKickerRate(){
  return m_kicker->GetSpeed();
}

void Shooter::TaskPeriodic(RobotMode mode) {
    m_flywheelRate->LogDouble(GetFlywheelRate());
    m_flywheelPowLog->LogDouble(m_flywheelMotorPrimary->GetOutputVoltage());
    m_flywheelAmpsLog->LogDouble(m_flywheelMotorPrimary->GetOutputCurrent());
    m_flywheelStateLog->LogPrintf("%d", m_flywheelState);
    m_speedSetpoint->LogDouble(m_flywheelSpeedSetpt);
    m_conveyorLog->LogDouble(m_ballConveyor->GetOutputCurrent());
    m_leftAgitatorLog->LogDouble(m_leftAgitator->GetOutputCurrent());
    m_rightAgitatorLog->LogDouble(m_rightAgitator->GetOutputCurrent());
    DBStringPrintf(DB_LINE5,"s_rate %2.1lf g %2.1lf", GetFlywheelRate(),
            m_flywheelSpeedSetpt);
    /*DBStringPrintf(DB_LINE3,"k_rate %2.1lf g %2.1lf", GetKickerRate(),
            m_kickerSpeedSetpt);*/
    /*DBStringPrintf(DB_LINE5,"flail %2.1lf conv %2.1lf",
                (m_leftAgitator->GetOutputCurrent() + m_rightAgitator->GetOutputCurrent()) / 2.0,
                m_ballConveyor->GetOutputCurrent());*/
//    DBStringPrintf(DB_LINE8,"shooterpow %2.1lf",
//            m_flywheelMotorPrimary->GetOutputVoltage());

    switch(m_shootingSequenceState){
      case idle: //no shooter subsystem movement
        StopAgitator();
        StopConveyor();
        break;
      case shooting: //auto shooting score sequence
        SetFlywheelSpeed(2970);
        if (OnTarget()) {
          m_drive->ArcadeDrive(0.0,0.0);
          StartAgitator(1.0, Side::right);
          StartAgitator(1.0, Side::left);
          StartConveyor(1.0);
        }
        break;
      case manual:
        break;
    }
    switch(m_flywheelState){
        case power: //open loop
            m_flywheelMotorPrimary->Set(m_flywheelPow);
            break;
        case notRunning:
            m_flywheelMotorPrimary->Set(0.0);
            break;
        case speed: //closed loop
            m_flywheelMotorPrimary->Set(m_flywheelSpeedSetpt);
            break;
    };
}

}
