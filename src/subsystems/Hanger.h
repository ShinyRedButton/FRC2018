/*
 * Hanger.h
 *
 *  Created on: Apr 17, 2016
 *      Author: Andrew
 */

#ifndef SRC_SUBSYSTEMS_HANGER_H_
#define SRC_SUBSYSTEMS_HANGER_H_

#include "lib/CoopTask.h"
#include "lib/TaskMgr.h"
#include "CANTalon.h"

using namespace frc;

namespace frc973 {

class Hanger : public CoopTask {
public:
	enum HangerState {
		start,
		autoHang,
		preHang,
		armed
	};

	Hanger(TaskMgr *scheduler);
	virtual ~Hanger();
	void TaskPeriodic(RobotMode mode);

	void SetHangerState(HangerState hangerState);
	void SetHangerClosedLoop(double position);
	void SetAutoHang();

private:
	TaskMgr *m_scheduler;
	CANTalon *m_crankMotor;
	CANTalon *m_crankMotorB;

	HangerState m_hangerState;

	double m_crankCurrent;
};

} /* namespace frc973 */

#endif /* SRC_SUBSYSTEMS_HANGER_H_ */
