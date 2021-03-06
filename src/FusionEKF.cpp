#include "FusionEKF.h"
#include "tools.h"
#include "Eigen/Dense"
#include <iostream>

using namespace std;
using Eigen::MatrixXd;
using Eigen::VectorXd;
using std::vector;

/*
 * Constructor.
 */
FusionEKF::FusionEKF() {
  is_initialized_ = false;

  previous_timestamp_ = 0;

  // initializing matrices
  R_laser_ = MatrixXd(2, 2);
  R_radar_ = MatrixXd(3, 3);
  H_laser_ = MatrixXd(2, 4);
  Hj_ = MatrixXd(3, 4);

  // measurement covariance matrix - laser
  R_laser_ << 0.0225, 0, 0, 0.0225;

  // measurement covariance matrix - radar
  R_radar_ << 0.09, 0, 0, 0, 0.0009, 0, 0, 0, 0.09;

  /**
  TODO:
    * Finish initializing the FusionEKF.
    * Set the process and measurement noises
  */

  // create a 4D state vector, we don't know yet the values of the x state
  // ekf_.x_ = VectorXd(4);

  // state covariance matrix P
  ekf_.P_ = MatrixXd(4, 4);
  ekf_.P_ << 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1000, 0, 0, 0, 0, 1000;

#if 0
	//measurement covariance
	ekf_.R_ = MatrixXd(2, 2);
	ekf_.R_ << 0.0225, 0,
			       0, 0.0225;
#endif

  // measurement matrix
  // ekf_.H_ = MatrixXd(2, 4);
  H_laser_ << 1, 0, 0, 0, 0, 1, 0, 0;

  // the initial transition matrix F_
  ekf_.F_ = MatrixXd(4, 4);
  ekf_.F_ << 1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0, 0, 0, 1;

  ekf_.Q_ = MatrixXd(4, 4);

  // set the acceleration noise components
  noise_ax = 9;
  noise_ay = 9;
}

/**
* Destructor.
*/
FusionEKF::~FusionEKF() {}

void FusionEKF::ProcessMeasurement(const MeasurementPackage &measurement_pack) {


  /*****************************************************************************
   *  Initialization
   ****************************************************************************/
  if (!is_initialized_) {
    /**
    TODO:
      * Initialize the state ekf_.x_ with the first measurement.
      * Create the covariance matrix.
      * Remember: you'll need to convert radar from polar to cartesian coordinates.
    */
    // first measurement
    cout << "EKF: " << endl;
    ekf_.x_ = VectorXd(4);
    ekf_.x_ << 1, 1, 1, 1;

    if (measurement_pack.sensor_type_ == MeasurementPackage::RADAR) {
      /**
      Convert radar from polar to cartesian coordinates and initialize state.
      */
      float ro = measurement_pack.raw_measurements_[0];
      float theta = measurement_pack.raw_measurements_[1];
      float ro_dot = measurement_pack.raw_measurements_[2];
      float px = ro * cos(theta);
      float py = ro * sin(theta);
      float vx = 0;
      float vy = 0;
      ekf_.x_ << px, py, vx, vy;
      //cout << "R: px " << px << " py " << py << " vx " << vx << " vy " << vy <<  " ro " << ro << " phi " << theta << " ro dot " << ro_dot << endl;
    } else if (measurement_pack.sensor_type_ == MeasurementPackage::LASER) {
      /**
      Initialize state.
      */
      float px = measurement_pack.raw_measurements_[0];
      float py = measurement_pack.raw_measurements_[1];
      float vx = 0;
      float vy = 0;
      ekf_.x_ << px, py, 0, 0;
      //cout << "L: px " << px << " py " << py << " vx " << vx << " vy " << vy << endl;
    }

    previous_timestamp_ = measurement_pack.timestamp_;
    // done initializing, no need to predict or update
    is_initialized_ = true;
    return;
  }

  /*****************************************************************************
   *  Prediction
   ****************************************************************************/

  /**
   TODO:
     * Update the state transition matrix F according to the new elapsed time.
      - Time is measured in seconds.
     * Update the process noise covariance matrix.
     * Use noise_ax = 9 and noise_ay = 9 for your Q matrix.
   */
  // compute the time elapsed between the current and previous measurements
  float dt = (measurement_pack.timestamp_ - previous_timestamp_) /
             1000000.0; // dt - expressed in seconds
  previous_timestamp_ = measurement_pack.timestamp_;

  // 1. Modify the F matrix so that the time is integrated
  ekf_.F_ << 1, 0, dt, 0, 0, 1, 0, dt, 0, 0, 1, 0, 0, 0, 0, 1;
  // 2. Set the process covariance matrix Q
  float dt_2 = dt * dt;
  float dt_3 = dt_2 * dt;
  float dt_4 = dt_2 * dt_2;
  // ekf_.Q_ = MatrixXd(4, 4);
  ekf_.Q_ << dt_4 * noise_ax / 4, 0, dt_3 * noise_ax / 2, 0, 0,
      dt_4 * noise_ay / 4, 0, dt_3 * noise_ay / 2, dt_3 * noise_ax / 2, 0,
      dt_2 * noise_ax, 0, 0, dt_3 * noise_ay / 2, 0, dt_2 * noise_ay;
  ekf_.Predict();
  //cout << "predict x_ = " << ekf_.x_ << endl;
  //cout << "predict P_ = " << ekf_.P_ << endl;

  /*****************************************************************************
   *  Update
   ****************************************************************************/

  /**
   TODO:
     * Use the sensor type to perform the update step.
     * Update the state and covariance matrices.
   */

  if (measurement_pack.sensor_type_ == MeasurementPackage::RADAR) {
    // Radar updates
#if 0
    float ro = measurement_pack.raw_measurements_[0];
    float theta = measurement_pack.raw_measurements_[1];
    float ro_dot = measurement_pack.raw_measurements_[2];
    float px = ro * cos(theta);
    float py = ro * sin(theta);
    float vx = 0;
    float vy = 0;
    cout << "R: px " << px << " py " << py << " vx " << vx << " vy " << vy <<  " ro " << ro << " phi " << theta << " ro dot " << ro_dot << endl;
#endif
    Hj_ = tools.CalculateJacobian(ekf_.x_);
    ekf_.H_ = Hj_;
    ekf_.R_ = R_radar_;
    ekf_.UpdateEKF(measurement_pack.raw_measurements_);
  } else {
    // Laser updates
#if 0
    float px = measurement_pack.raw_measurements_[0];
    float py = measurement_pack.raw_measurements_[1];
    float vx = 0;
    float vy = 0;
    cout << "L: px " << px << " py " << py << " vx " << vx << " vy " << vy << endl;
#endif
    ekf_.H_ = H_laser_;
    ekf_.R_ = R_laser_;
    ekf_.Update(measurement_pack.raw_measurements_);
  }

  // print the output
  cout << "x_ = " << ekf_.x_ << endl;
  cout << "P_ = " << ekf_.P_ << endl;
}
