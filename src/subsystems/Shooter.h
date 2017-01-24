/*
 * Shooter.h
 *
 *  Created on: Oct 9, 2015
 *      Author: Andrew
 */

#pragma once

#include "WPILib.h"
#include "lib/CoopTask.h"
#include "CANTalon.h"

using namespace frc;

namespace frc973 {

class TaskMgr;
class LogSpreadsheet;

/**
 * Open loop control on flywheel at the moment... will do fine tuning
 * once it's shown that everything else works
 */
class Shooter : public CoopTask
{
public:
	Shooter(TaskMgr *scheduler, LogSpreadsheet *logger);
	virtual ~Shooter();
	void TaskPeriodic(RobotMode mode);
	void SetFlywheelPow(double pow);
	void SetFlywheelStop();
	void SetFLywheelSpeed(double speed);

	double GetFlywheelRate();

	static constexpr int DEFAULT_FLYWHEEL_SPEED_SETPOINT = 2700;

enum FlywheelState {
	power,
	notRunning,
	speed
};

private:
	TaskMgr *m_scheduler;

	CANTalon *m_flywheelMotorPrimary;
	CANTalon *m_flywheelMotorReplica;

	double m_flywheelPow;

	FlywheelState m_flywheelState;

	LogCell *m_flywheelRate;
	LogCell *m_flywheelPowLog;
	LogCell *m_flywheelStateLog;
	LogCell *m_speedSetpoint;


};

}
