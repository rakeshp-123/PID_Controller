#include "PID.h"

using namespace std;

/*
* TODO: Complete the PID class.
*/

PID::PID() {}

PID::~PID() {}

void PID::Init(double p, double i, double d) {
   Kp = p;
   Ki = i;
   Kd = d;

   p_error = 0.0;
   i_error = 0.0;
   d_error = 0.0;
}

void PID::UpdateError(double cte) {	
   /*
     Calculate the error terms 
   */
   d_error = cte - p_error;
   p_error = cte;
   i_error += cte;

}



double PID::TotalError() {
   double steering_angle = (-Kp * p_error) - (Kd * d_error) - (Ki * i_error);
   return steering_angle;
}

