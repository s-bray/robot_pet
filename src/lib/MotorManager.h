#ifndef MOTOR_MANAGER_H
#define MOTOR_MANAGER_H

class MotorManager
{
private:
  int in1, in2, in3, in4;

public:
  MotorManager(int m1, int m2, int m3, int m4);
  void begin();
  void forward();
  void backward();
  void left();
  void right();
  void stop();
};

#endif