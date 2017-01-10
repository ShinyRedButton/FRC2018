#ifndef SRC_SUBSYSTEMS_TURRET_H_
#define SRC_SUBSYSTEMS_TURRET_H_

#include "RobotInfo.h"
#include "WPILib.h"
#include "CANTalon.h"
#include "lib/CoopTask.h"

using namespace frc;

namespace frc973 {

  class TaskMgr;

  class Turret: public CoopTask{
    public:
      Turret(TaskMgr *m_scheduler, LogSpreadsheet *logger);
      virtual ~Turret();

      enum TurretState{
        running,
        notRunning,
        vision
      };
      void SetTurretPosition(double position);
      double GetTurretPosition();
      void StopTurret();
      void SetTurretPower(double power);
      void SetTurretMode(TurretState turretState);
      void SetTurretAutoTarget();

      void TaskPeriodic();

    private:
      CANTalon *m_turretMotor;
      TaskMgr *m_scheduler;

      TurretState m_turretState;

      double m_turretPos;
      double m_turretPower;

  };
}

#endif /* SRC_SUBSYSTEMS_TURRET_H_*/