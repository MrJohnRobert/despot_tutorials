#include <despot/evaluator.h>
#include <laser_tag.h>
#include <ros/ros.h>
#include <laser_tag/TagActionObs.h>

// for tests
#include <iostream>

using namespace despot;

class LaserTagWorld: public World {
public:


    ros::NodeHandle nh;
    ros::ServiceClient client;
    //Establish connection with simulator or system
    bool Connect(){
        // initilize ROS node. Should be done just once
        int argc;
        char ** argv;
        ros::init(argc, argv, "test_laser_tag");

        // wait for controller service to show up (waits forever)
        ros::service::waitForService("laser_tag_action_obs", -1);
        
        // setup service client
        client = nh.serviceClient<laser_tag::TagActionObs>("laser_tag_action_obs");
    }
    //Initialize or reset the environment (for simulators or POMDP world only), return the start state of the system if applicable
    State* Initialize(){
        return NULL;
    }
    //Get the state of the system (only applicable for simulators or POMDP world)
    State* GetCurrentState(){
        return NULL;
    }
    //Send action to be executed by the system, receive observations terminal signals from the system
    bool ExecuteAction(ACT_TYPE action, OBS_TYPE& obs){
        // send action & receive observation
        laser_tag::TagActionObs srv;
        srv.request.action = (int) action; // actions: 0 - North, 1 - East, 2 - South, 3 - West, 4 - Tag
        if (client.call(srv))
        {
          // observations after executing actions
          std::vector<int> laser_obs = srv.response.observations;

          // print observations (metric readings rounded to the nearest integer)
          ROS_INFO("Laser Observations");
          ROS_INFO("North: %d"    , laser_obs[0]);
          ROS_INFO("East: %d"     , laser_obs[1]);
          ROS_INFO("South: %d"    , laser_obs[2]);
          ROS_INFO("West: %d"     , laser_obs[3]);
          ROS_INFO("NorthEast: %d", laser_obs[4]);
          ROS_INFO("SouthEast: %d", laser_obs[5]);
          ROS_INFO("SouthWest: %d", laser_obs[6]);
          ROS_INFO("NorthWest: %d", laser_obs[7]);
          
          for (int dir = 0; dir < 8; dir++) {
             LaserTag::SetReading(obs, laser_obs[dir], dir);
          }
          if(action==4)
            return 1;
          else
            return 0;
        }
        else
        {
          ROS_ERROR("Invalid Action OR Invalid Tag");
          return 0;
        }
    }
};

class MyEvaluator: public Evaluator {
public:
  MyEvaluator() {
  }

  DSPOMDP* InitializeModel(option::Option* options) {
    DSPOMDP* model = !options[E_PARAMS_FILE] ?
      new LaserTag() : new LaserTag(options[E_PARAMS_FILE].arg);
    return model;
  }

  World* InitializeWorld(std::string& world_type, DSPOMDP* model, option::Option* options)
  {
      //Create a custom world as defined and implemented by the user
      LaserTagWorld* world = new LaserTagWorld();
      //Establish connection with external system
      world->Connect();
      //Initialize the state of the external system
      world->Initialize();
      //Inform despot the type of world
      world_type = "simulator";
      return world; 
  }

  void InitializeDefaultParameters() {
    Globals::config.pruning_constant = 0.01;
  }

  std::string ChooseSolver(){
	  return "DESPOT";
  }
};

int main(int argc, char* argv[]) {
  return MyEvaluator().runEvaluation(argc, argv);
}
