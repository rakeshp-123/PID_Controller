#include <uWS/uWS.h>
#include <iostream>
#include <unistd.h>
#include "json.hpp"
#include "PID.h"
#include <math.h>

// for convenience
using json = nlohmann::json;

// For converting back and forth between radians and degrees.
constexpr double pi() { return M_PI; }
double deg2rad(double x) { return x * pi() / 180; }
double rad2deg(double x) { return x * 180 / pi(); }

// Checks if the SocketIO event has JSON data.
// If there is data the JSON object in string format will be returned,
// else the empty string "" will be returned.
std::string hasData(std::string s) {
  auto found_null = s.find("null");
  auto b1 = s.find_first_of("[");
  auto b2 = s.find_last_of("]");
  if (found_null != std::string::npos) {
    return "";
  }
  else if (b1 != std::string::npos && b2 != std::string::npos) {
    return s.substr(b1, b2 - b1 + 1);
  }
  return "";
}

int main(int argc, char* argv[])
{
  uWS::Hub h;
  bool calculate_tau = false;
  int num_loop = 0;
  double total_error = 0.0;
  double p[3] = {0.1, 0.0, 1.0};
  double dp[3] = {0.01, 0.0001, 0.2};
  //double dp[3] = {1, 1, 1};
  int total_loop = 700;
  bool p_limit_top = true;
  bool p_limit_down = true;
  double best_error = 99999.1;
  double tol = 0.2;
  int p_idx = 0;
  
  PID pid;
  if(argc == 5)
  {
    calculate_tau = (atoi(argv[4])==1?true:false); 
  }
  if(calculate_tau && argc > 1)
  {
	p[0] = atof(argv[1]);
	p[1] = atof(argv[2]);
	p[2] = atof(argv[3]);
  }
  else
  {
    p[0] = 0.121;
    p[1] = 0.00021;
    p[2]=1.2;
  }
  // TODO: Initialize the pid variable.
  pid.Init(p[0], p[1], p[2]);
  

  h.onMessage([&](uWS::WebSocket<uWS::SERVER> ws, char *data, size_t length, uWS::OpCode opCode) {
    // "42" at the start of the message means there's a websocket message event.
    // The 4 signifies a websocket message
    // The 2 signifies a websocket event
    if (length && length > 2 && data[0] == '4' && data[1] == '2')
    {
      auto s = hasData(std::string(data).substr(0, length));
      if (s != "") {
        auto j = json::parse(s);
        std::string event = j[0].get<std::string>();
        if (event == "telemetry") {
          // j[1] is the data JSON object
          double cte = std::stod(j[1]["cte"].get<std::string>());
          double speed = std::stod(j[1]["speed"].get<std::string>());
          double angle = std::stod(j[1]["steering_angle"].get<std::string>());
          double steer_value;
          json msgJson;
          /*
          * TODO: Calcuate steering value here, remember the steering value is
          * [-1, 1].
          * NOTE: Feel free to play around with the throttle and speed. Maybe use
          * another PID controller to control the speed!
          */
          if(!calculate_tau)
	  {
		std::cout << "p_error: " << pid.p_error << "i_error: " << pid.i_error << "d_error: "<<pid.d_error<< std::endl;
    		pid.UpdateError(cte);
      	  	steer_value = pid.TotalError();
		
		std::cout << "CTE: " << cte << " Steering Value: " << steer_value << std::endl;

          	
          	msgJson["steering_angle"] = steer_value;
          	msgJson["throttle"] = 0.4;
          	auto msg = "42[\"steer\"," + msgJson.dump() + "]";
          	//std::cout << msg << std::endl;
          	ws.send(msg.data(), msg.length(), uWS::OpCode::TEXT);

	  }

	  else
	  {
		if(num_loop == 0)
		{
			pid.Init(p[0], p[1], p[2]);
		}
		
		
		pid.UpdateError(cte);
      	  	steer_value = pid.TotalError();
		total_error += pow(cte,  2);
		num_loop++;
                //std::cout<< "loop_count: " << num_loop <<std::endl;

                if(num_loop <= total_loop)
		{
			msgJson["steering_angle"] = steer_value;
          		msgJson["throttle"] = 0.3;
          		auto msg = "42[\"steer\"," + msgJson.dump() + "]";
          		//std::cout << msg << std::endl;
                        
          		ws.send(msg.data(), msg.length(), uWS::OpCode::TEXT);
		}
		else
		{
			std::cout << "dp: " << dp[0] << " di: " << dp[1] << " dd: "<<dp[2]<< std::endl;
			if(p_limit_top == true)
			{
				p[p_idx] += dp[p_idx];
				p_limit_top = false;
			}		
			else	
			{
				double err = (double)total_error/total_loop;
				if(err < best_error)
				{
					best_error = err;
					dp[p_idx] *= 1.1;
                    std::cout<<"in first"<<std::endl;
					p_idx = (p_idx + 1)  %  3;
					p_limit_top = true;
					p_limit_down = true;
                   std::cout << "p[0]: " << p[0] << " p[1]: " << p[1] << " p[2]: "<<p[2]<< std::endl;
				}
				else
				{
					if(p_limit_down == true)
					{
						p[p_idx] -= 2 * dp[p_idx];
						p_limit_down = false;
					}
					else
					{
						p[p_idx] += dp[p_idx];
						dp[p_idx] *= 0.9;
                       std::cout<<"in second"<<std::endl;
						p_idx = (p_idx + 1)  %  3;
						p_limit_top = true;
						p_limit_down = true;
                        std::cout << "p[0]: " << p[0] << " p[1]: " << p[1] << " p[2]: "<<p[2]<< std::endl;
					}
				}
			
			}
			num_loop = 0;
			total_error = 0.0;
			std::cout<<"p_idx: "<< p_idx<<std::endl;
			if((dp[0] + dp[2] + dp[1]) > tol)
			{
				std::string msg = "42[\"reset\",{}]";
				ws.send(msg.data(), msg.length(), uWS::OpCode::TEXT);
			}
			else
			{
				std::cout<<"best Kp: "<<p[0]<<" best Ki: "<<p[1]<<" best Kd: "<<p[2];
                usleep(100000000);
			}
		}
	  }
 	  
	  

          // DEBUG
          
        }
      } else {
        // Manual driving
        std::string msg = "42[\"manual\",{}]";
        ws.send(msg.data(), msg.length(), uWS::OpCode::TEXT);
      }
    }
  });

  // We don't need this since we're not using HTTP but if it's removed the program
  // doesn't compile :-(
  h.onHttpRequest([](uWS::HttpResponse *res, uWS::HttpRequest req, char *data, size_t, size_t) {
    const std::string s = "<h1>Hello world!</h1>";
    if (req.getUrl().valueLength == 1)
    {
      res->end(s.data(), s.length());
    }
    else
    {
      // i guess this should be done more gracefully?
      res->end(nullptr, 0);
    }
  });

  h.onConnection([&h](uWS::WebSocket<uWS::SERVER> ws, uWS::HttpRequest req) {
    std::cout << "Connected!!!" << std::endl;
  });

  h.onDisconnection([&h](uWS::WebSocket<uWS::SERVER> ws, int code, char *message, size_t length) {
    ws.close();
    std::cout << "Disconnected" << std::endl;
  });

  int port = 4567;
  if (h.listen(port))
  {
    std::cout << "Listening to port " << port << std::endl;
  }
  else
  {
    std::cerr << "Failed to listen to port" << std::endl;
    return -1;
  }
  h.run();
}
