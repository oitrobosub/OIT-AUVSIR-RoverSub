//Author: Tyler Lucas
//Creation Date: 12/28/2022
using System.Net;
using System.Net.Sockets;
using System.Threading;
using System.Text;

namespace FakeBot
{
    /// <summary>
    /// The BotServer class creates a TCP server that will emulate a robot. It will
    /// send data to the connected client (Senior Project program) in place of a 
    /// connection to a real robot with IMUs.
    /// </summary>
    public class BotServer
    {
        private static int sendCount = -1;

        private struct IMU
        {
            public int IMUID;
            public string IMUType;
            public string IMUName;
            public float Weight;
            public string ? data;
        }
        static void Main()
        {
            //local
            IPAddress ipa = IPAddress.Parse("127.0.0.1");
            int port = 8888;

            //Listens for a connection on local
            TcpListener listener = new TcpListener(ipa, port);
            listener.Start();

            Console.WriteLine("Listening on port: {0}", port);

            bool done = false;

            while(!done)
            {
                TcpClient tcpClient = listener.AcceptTcpClient();

                //Multiple threads in case needed for testing
                Thread thread = new Thread(() => ClientHandler(tcpClient));
                thread.Start();
            }
        }

        /// <summary>
        /// Provides the correct response string with data from the IMUs
        /// </summary>
        /// <param name="accessType"></param>
        /// <param name="displayConnected"></param>
        /// <returns>response string</returns>
        static byte[] AccessIMUs(string accessType, bool displayConnected)
        {
            //NOTE: This list and future data parsing/sending assume the IMUName is a field the club can modify to include "-AUVSIR-SIDEOFBOT"
            //           where SIDEOFBOT is either F/R/P/S for Frontal/Rear/Port/Starboard
            List<IMU> IMUs = new List<IMU>();

            //-------------------------------------------------------------------------------------
            ///Instant initial position sets - There is a front set, rear set, port set, and starboard set
            //WitMotion WT901C TTL 9 Axis IMU Sensor Tilt Angle Roll Pitch Yaw + Acceleration + Gyroscope + Magnetometer MPU9250 on PC/Android/MCU
            IMU frontalGyroAccMag = new IMU
            {
                IMUType = "Gyro/Accelerometer/Magnetometer",
                IMUName = "WitMotion-WT901C-AUVSIR-F",
                Weight = .8F,
                //Accelerating forward, Rotation(deg), acceleration, magnetic field
                data = "Rx:150 Ry:-5 Rz:20 Ax:.98 Ay:.02 Az:-.91 Mx:45 My:35 Mz:-60"
            };
            IMUs.Add(frontalGyroAccMag);
            //WitMotion WT901C TTL 9 Axis IMU Sensor Tilt Angle Roll Pitch Yaw + Acceleration + Gyroscope + Magnetometer MPU9250 on PC/Android/MCU
            IMU rearwardGyroAccMag = new IMU
            {
                IMUType = "Gyro/Accelerometer/Magnetometer",
                IMUName = "WitMotion-WT901C-AUVISR-R",
                Weight = .8F,
                //Accelerating forward, Rotation(deg), acceleration, magnetic field
                data = "Rx:145 Ry:-4 Rz:23 Ax:.99 Ay:.02 Az:-.89 Mx:48 My:37 Mz:-55"
            };
            IMUs.Add(rearwardGyroAccMag);
            //Bosch-BMP085 barometer chipset
            IMU frontalBaro = new IMU
            {
                IMUType = "Barrometer",
                IMUName = "Bosch-BMP085-F",
                Weight = .91F,
                data = "Bar:.99"
            };
            IMUs.Add(frontalBaro);
            //Bosch-BMP085 barometer chipset
            IMU rearwardBaro = new IMU
            {
                IMUType = "Barrometer",
                IMUName = "Bosch-BMP085-R",
                Weight = .95F,
                data = "Bar:.97"
            };
            IMUs.Add(rearwardBaro);
            //Lidar-Lite v3 
            IMU frontalAltimeter = new IMU
            {
                IMUType = "Altimeter",
                IMUName = "Lidar-Lite-v3-F",
                Weight = .96F,
                data = "H:1.5"
            };
            IMUs.Add(frontalAltimeter);
            //Lidar-Lite v3 
            IMU rearwardAltimeter = new IMU
            {
                IMUType = "Altimeter",
                IMUName = "Lidar-Lite-v3-R",
                Weight = .97F,
                data = "H:1.4"
            };
            IMUs.Add(rearwardAltimeter);
            //Lidar-Lite v3 
            IMU starboardAltimeter = new IMU
            {
                IMUType = "Altimeter",
                IMUName = "Lidar-Lite-v3-S",
                Weight = .95F,
                data = "H:1.5"
            };
            IMUs.Add(starboardAltimeter);
            //Lidar-Lite v3 
            IMU portAltimeter = new IMU
            {
                IMUType = "Altimeter",
                IMUName = "Lidar-Lite-v3-P",
                Weight = .965F,
                data = "H:1.4"
            };
            IMUs.Add(portAltimeter);
            //Garmin Striker 4 Fishfinder
            IMU frontalSonar = new IMU
            {
                IMUType = "Sonar",
                IMUName = "Garmin-Striker-4-Fishfinder-F",
                Weight = .98F,
                //object depth, range, angle
                data = "Sd:15.5 Sr:80 Sa:240"
            };
            IMUs.Add(frontalSonar);
            //Bosch Sensortec BMG250 MEMS
            IMU starboardGyro = new IMU
            {
                IMUType = "Gyro",
                IMUName = "Bosch-Sensortec-BMG250-MEMS-S",
                Weight = .82F,
                data = "Rx:147 Ry:-4.5 Rz:24"
            };
            IMUs.Add(starboardGyro);
            //Bosch Sensortec BMG250 MEMS
            IMU portGyro = new IMU
            {
                IMUType = "Gyro",
                IMUName = "Bosch-Sensortec-BMG250-MEMS-P",
                Weight = .8F,
                data = "Rx:151 Ry:-4 Rz:21"
            };
            IMUs.Add(portGyro);
            //-------------------------------------------------------------------------------------
            //-------------------------------------------------------------------------------------
            ///Forward moving sets
            //Starts with previous stationary sets (IMUs list) and moves from there
            List<IMU> fpIMUs = new List<IMU>();//primary motion

            //WitMotion WT901C TTL 9 Axis IMU Sensor Tilt Angle Roll Pitch Yaw + Acceleration + Gyroscope + Magnetometer MPU9250 on PC/Android/MCU
            frontalGyroAccMag.data = "Rx:145 Ry:-5 Rz:20 Ax:1.02 Ay:.02 Az:-.94 Mx:45 My:35 Mz:-60";
            fpIMUs.Add(frontalGyroAccMag);
            //WitMotion WT901C TTL 9 Axis IMU Sensor Tilt Angle Roll Pitch Yaw + Acceleration + Gyroscope + Magnetometer MPU9250 on PC/Android/MCU
            rearwardGyroAccMag.data = "Rx:142 Ry:-4 Rz:23 Ax:1.04 Ay:.02 Az:-.94 Mx:48 My:37 Mz:-55";
            fpIMUs.Add(rearwardGyroAccMag);

            //Bosch-BMP085 barometer chipset
            frontalBaro.data = "Bar:.98";
            fpIMUs.Add(frontalBaro);
            //Bosch-BMP085 barometer chipset
            rearwardBaro.data = "Bar:.9712";
            fpIMUs.Add(rearwardBaro);

            //Lidar-Lite v3 
            frontalAltimeter.data = "H:1.45";
            fpIMUs.Add(frontalAltimeter);
            //Lidar-Lite v3 
            rearwardAltimeter.data = "H:1.51";
            fpIMUs.Add(rearwardAltimeter);
            //Lidar-Lite v3 
            starboardAltimeter.data = "H:1.48";
            fpIMUs.Add(starboardAltimeter);
            //Lidar-Lite v3 
            portAltimeter.data = "H:1.47";
            fpIMUs.Add(portAltimeter);

            //Garmin Striker 4 Fishfinder
            frontalSonar.data = "Sd:15.5 Sr:70 Sa:221";
            fpIMUs.Add(frontalSonar);

            //Bosch Sensortec BMG250 MEMS
            starboardGyro.data = "Rx:140 Ry:-4.5 Rz:24";
            fpIMUs.Add(starboardGyro);
            //Bosch Sensortec BMG250 MEMS
            portGyro.data = "Rx:144 Ry:-4 Rz:21";
            fpIMUs.Add(portGyro);

            List<IMU> fsIMUs = new List<IMU>();//second motion

            //WitMotion WT901C TTL 9 Axis IMU Sensor Tilt Angle Roll Pitch Yaw + Acceleration + Gyroscope + Magnetometer MPU9250 on PC/Android/MCU
            frontalGyroAccMag.data = "Rx:146 Ry:-5 Rz:20 Ax:1.1 Ay:.02 Az:-.99 Mx:45 My:35 Mz:-60";
            fsIMUs.Add(frontalGyroAccMag);
            //WitMotion WT901C TTL 9 Axis IMU Sensor Tilt Angle Roll Pitch Yaw + Acceleration + Gyroscope + Magnetometer MPU9250 on PC/Android/MCU
            rearwardGyroAccMag.data = "Rx:140 Ry:-4 Rz:23 Ax:1.15 Ay:.02 Az:-1.0 Mx:48 My:37 Mz:-55";
            fsIMUs.Add(rearwardGyroAccMag);

            //Bosch-BMP085 barometer chipset
            frontalBaro.data = "Bar:.9857";
            fsIMUs.Add(frontalBaro);
            //Bosch-BMP085 barometer chipset
            rearwardBaro.data = "Bar:.992";
            fsIMUs.Add(rearwardBaro);

            //Lidar-Lite v3 
            frontalAltimeter.data = "H:1.44";
            fsIMUs.Add(frontalAltimeter);
            //Lidar-Lite v3 
            rearwardAltimeter.data = "H:1.50";
            fsIMUs.Add(rearwardAltimeter);
            //Lidar-Lite v3 
            starboardAltimeter.data = "H:1.47";
            fsIMUs.Add(starboardAltimeter);
            //Lidar-Lite v3 
            portAltimeter.data = "H:1.465";
            fsIMUs.Add(portAltimeter);

            //Garmin Striker 4 Fishfinder
            frontalSonar.data = "Sd:15.5 Sr:60 Sa:204";
            fsIMUs.Add(frontalSonar);

            //Bosch Sensortec BMG250 MEMS
            starboardGyro.data = "Rx:139 Ry:-4.5 Rz:24";
            fsIMUs.Add(starboardGyro);
            //Bosch Sensortec BMG250 MEMS
            portGyro.data = "Rx:142 Ry:-4 Rz:21";
            fsIMUs.Add(portGyro);

            List<IMU> ftIMUs = new List<IMU>();//third motion

            //WitMotion WT901C TTL 9 Axis IMU Sensor Tilt Angle Roll Pitch Yaw + Acceleration + Gyroscope + Magnetometer MPU9250 on PC/Android/MCU
            frontalGyroAccMag.data = "Rx:147 Ry:-5 Rz:20 Ax:1.16 Ay:.02 Az:-1.05 Mx:45 My:35 Mz:-60";
            ftIMUs.Add(frontalGyroAccMag);
            //WitMotion WT901C TTL 9 Axis IMU Sensor Tilt Angle Roll Pitch Yaw + Acceleration + Gyroscope + Magnetometer MPU9250 on PC/Android/MCU
            rearwardGyroAccMag.data = "Rx:142 Ry:-4 Rz:23 Ax:1.15 Ay:.02 Az:-1.03 Mx:48 My:37 Mz:-55";
            ftIMUs.Add(rearwardGyroAccMag);

            //Bosch-BMP085 barometer chipset
            frontalBaro.data = "Bar:.9799";
            ftIMUs.Add(frontalBaro);
            //Bosch-BMP085 barometer chipset
            rearwardBaro.data = "Bar:.989";
            ftIMUs.Add(rearwardBaro);

            //Lidar-Lite v3 
            frontalAltimeter.data = "H:1.435";
            ftIMUs.Add(frontalAltimeter);
            //Lidar-Lite v3 
            rearwardAltimeter.data = "H:1.51";
            ftIMUs.Add(rearwardAltimeter);
            //Lidar-Lite v3 
            starboardAltimeter.data = "H:1.46";
            ftIMUs.Add(starboardAltimeter);
            //Lidar-Lite v3 
            portAltimeter.data = "H:1.5";
            ftIMUs.Add(portAltimeter);

            //Garmin Striker 4 Fishfinder
            frontalSonar.data = "Sd:15.5 Sr:50 Sa:185";
            ftIMUs.Add(frontalSonar);

            //Bosch Sensortec BMG250 MEMS
            starboardGyro.data = "Rx:138.1 Ry:-4.5 Rz:24";
            ftIMUs.Add(starboardGyro);
            //Bosch Sensortec BMG250 MEMS
            portGyro.data = "Rx:141 Ry:-4 Rz:21";
            ftIMUs.Add(portGyro);

            //-------------------------------------------------------------------------------------
            //-------------------------------------------------------------------------------------
            ///Left turn sets
            //Starts with previous stationary sets (IMUs list) and moves from there
            //NOTE:Ry and Rz values are based on the yaw inflicted by the turn. These are best guesses.
            List<IMU> lpIMUs = new List<IMU>();//primary motion
            //WitMotion WT901C TTL 9 Axis IMU Sensor Tilt Angle Roll Pitch Yaw + Acceleration + Gyroscope + Magnetometer MPU9250 on PC/Android/MCU
            frontalGyroAccMag.data = "Rx:145 Ry:40 Rz:-25 Ax:1.02 Ay:.02 Az:-.94 Mx:45 My:30 Mz:-50";
            lpIMUs.Add(frontalGyroAccMag);
            //WitMotion WT901C TTL 9 Axis IMU Sensor Tilt Angle Roll Pitch Yaw + Acceleration + Gyroscope + Magnetometer MPU9250 on PC/Android/MCU
            rearwardGyroAccMag.data = "Rx:142 Ry:-49 Rz:-22 Ax:1.04 Ay:.02 Az:-.94 Mx:48 My:31 Mz:-48.5";
            lpIMUs.Add(rearwardGyroAccMag);

            //Bosch-BMP085 barometer chipset
            frontalBaro.data = "Bar:.982";
            lpIMUs.Add(frontalBaro);
            //Bosch-BMP085 barometer chipset
            rearwardBaro.data = "Bar:.988";
            lpIMUs.Add(rearwardBaro);

            //Lidar-Lite v3 
            frontalAltimeter.data = "H:1.45";
            lpIMUs.Add(frontalAltimeter);
            //Lidar-Lite v3 
            rearwardAltimeter.data = "H:1.50";
            lpIMUs.Add(rearwardAltimeter);
            //Lidar-Lite v3 
            starboardAltimeter.data = "H:1.47";
            lpIMUs.Add(starboardAltimeter);
            //Lidar-Lite v3 
            portAltimeter.data = "H:1.49";
            lpIMUs.Add(portAltimeter);

            //Garmin Striker 4 Fishfinder
            frontalSonar.data = "Sd:15.5 Sr:70 Sa:221";
            lpIMUs.Add(frontalSonar);

            //Bosch Sensortec BMG250 MEMS
            starboardGyro.data = "Rx:144 Ry:102 Rz:-21";
            lpIMUs.Add(starboardGyro);
            //Bosch Sensortec BMG250 MEMS
            portGyro.data = "Rx:140 Ry:106 Rz:-24";
            lpIMUs.Add(portGyro);

            List<IMU> lsIMUs = new List<IMU>();//second motion
            //WitMotion WT901C TTL 9 Axis IMU Sensor Tilt Angle Roll Pitch Yaw + Acceleration + Gyroscope + Magnetometer MPU9250 on PC/Android/MCU
            frontalGyroAccMag.data = "Rx:145 Ry:60 Rz:-40 Ax:.99 Ay:.03 Az:-.91 Mx:45 My:27 Mz:-42";
            lsIMUs.Add(frontalGyroAccMag);
            //WitMotion WT901C TTL 9 Axis IMU Sensor Tilt Angle Roll Pitch Yaw + Acceleration + Gyroscope + Magnetometer MPU9250 on PC/Android/MCU
            rearwardGyroAccMag.data = "Rx:142 Ry:-64 Rz:-37 Ax:.96 Ay:.02 Az:-.93 Mx:48 My:26.4 Mz:-40";
            lsIMUs.Add(rearwardGyroAccMag);

            //Bosch-BMP085 barometer chipset
            frontalBaro.data = "Bar:.99";
            lsIMUs.Add(frontalBaro);
            //Bosch-BMP085 barometer chipset
            rearwardBaro.data = "Bar:.992";
            lsIMUs.Add(rearwardBaro);

            //Lidar-Lite v3 
            frontalAltimeter.data = "H:1.46";
            lsIMUs.Add(frontalAltimeter);
            //Lidar-Lite v3 
            rearwardAltimeter.data = "H:1.49";
            lsIMUs.Add(rearwardAltimeter);
            //Lidar-Lite v3 
            starboardAltimeter.data = "H:1.46";
            lsIMUs.Add(starboardAltimeter);
            //Lidar-Lite v3 
            portAltimeter.data = "H:1.50";
            lsIMUs.Add(portAltimeter);

            //Garmin Striker 4 Fishfinder
            frontalSonar.data = "Sd:17 Sr:65 Sa:212";
            lsIMUs.Add(frontalSonar);

            //Bosch Sensortec BMG250 MEMS
            starboardGyro.data = "Rx:143 Ry:165 Rz:-36";
            lsIMUs.Add(starboardGyro);
            //Bosch Sensortec BMG250 MEMS
            portGyro.data = "Rx:139 Ry:172 Rz:-39";
            lsIMUs.Add(portGyro);

            //-------------------------------------------------------------------------------------
            //-------------------------------------------------------------------------------------
            ///Right turn sets
            //Starts with previous stationary sets (IMUs list) and moves from there
            List<IMU> rpIMUs = new List<IMU>();//primary motion

            //WitMotion WT901C TTL 9 Axis IMU Sensor Tilt Angle Roll Pitch Yaw + Acceleration + Gyroscope + Magnetometer MPU9250 on PC/Android/MCU
            frontalGyroAccMag.data = "Rx:145 Ry:-50 Rz:62 Ax:.98 Ay:.03 Az:-.89 Mx:45 My:41 Mz:-64";
            rpIMUs.Add(frontalGyroAccMag);
            //WitMotion WT901C TTL 9 Axis IMU Sensor Tilt Angle Roll Pitch Yaw + Acceleration + Gyroscope + Magnetometer MPU9250 on PC/Android/MCU
            rearwardGyroAccMag.data = "Rx:142 Ry:39 Rz:64 Ax:1.01 Ay:.01 Az:-.9 Mx:48 My:43.2 Mz:-59.9";
            rpIMUs.Add(rearwardGyroAccMag);

            //Bosch-BMP085 barometer chipset
            frontalBaro.data = "Bar:.988";
            rpIMUs.Add(frontalBaro);
            //Bosch-BMP085 barometer chipset
            rearwardBaro.data = "Bar:.982";
            rpIMUs.Add(rearwardBaro);

            //Lidar-Lite v3 
            frontalAltimeter.data = "H:1.46";
            rpIMUs.Add(frontalAltimeter);
            //Lidar-Lite v3 
            rearwardAltimeter.data = "H:1.50";
            rpIMUs.Add(rearwardAltimeter);
            //Lidar-Lite v3 
            starboardAltimeter.data = "H:1.47";
            rpIMUs.Add(starboardAltimeter);
            //Lidar-Lite v3 
            portAltimeter.data = "H:1.48";
            rpIMUs.Add(portAltimeter);

            //Garmin Striker 4 Fishfinder
            frontalSonar.data = "Sd:21 Sr:82 Sa:256";
            rpIMUs.Add(frontalSonar);

            //Bosch Sensortec BMG250 MEMS
            starboardGyro.data = "Rx:140 Ry:-3.5 Rz:-21";
            rpIMUs.Add(starboardGyro);
            //Bosch Sensortec BMG250 MEMS
            portGyro.data = "Rx:144 Ry:-4.5 Rz:-24";
            rpIMUs.Add(portGyro);

            List<IMU> rsIMUs = new List<IMU>();//second motion

            //WitMotion WT901C TTL 9 Axis IMU Sensor Tilt Angle Roll Pitch Yaw + Acceleration + Gyroscope + Magnetometer MPU9250 on PC/Android/MCU
            frontalGyroAccMag.data = "Rx:145 Ry:-65 Rz:73 Ax:1.01 Ay:.027 Az:-.87 Mx:45 My:52 Mz:-69";
            rsIMUs.Add(frontalGyroAccMag);
            //WitMotion WT901C TTL 9 Axis IMU Sensor Tilt Angle Roll Pitch Yaw + Acceleration + Gyroscope + Magnetometer MPU9250 on PC/Android/MCU
            rearwardGyroAccMag.data = "Rx:142 Ry:55 Rz:75 Ax:.98 Ay:.02 Az:-.92 Mx:48 My:54.1 Mz:-64.9";
            rsIMUs.Add(rearwardGyroAccMag);

            //Bosch-BMP085 barometer chipset
            frontalBaro.data = "Bar:.991";
            rsIMUs.Add(frontalBaro);
            //Bosch-BMP085 barometer chipset
            rearwardBaro.data = "Bar:.987";
            rsIMUs.Add(rearwardBaro);

            //Lidar-Lite v3 
            frontalAltimeter.data = "H:1.48";
            rsIMUs.Add(frontalAltimeter);
            //Lidar-Lite v3 
            rearwardAltimeter.data = "H:1.51";
            rsIMUs.Add(rearwardAltimeter);
            //Lidar-Lite v3 
            starboardAltimeter.data = "H:1.46";
            rsIMUs.Add(starboardAltimeter);
            //Lidar-Lite v3 
            portAltimeter.data = "H:1.49";
            rsIMUs.Add(portAltimeter);

            //Garmin Striker 4 Fishfinder
            frontalSonar.data = "Sd:20.8 Sr:81.5 Sa:254";
            rsIMUs.Add(frontalSonar);

            //Bosch Sensortec BMG250 MEMS
            starboardGyro.data = "Rx:140 Ry:3.5 Rz:-36";
            rsIMUs.Add(starboardGyro);
            //Bosch Sensortec BMG250 MEMS
            portGyro.data = "Rx:144 Ry:4.5 Rz:-39";
            rsIMUs.Add(portGyro);

            //-------------------------------------------------------------------------------------
            //-------------------------------------------------------------------------------------
            ///Submerging sets
            //Starts with previous stationary sets (IMUs list) and moves from there
            List<IMU> spIMUs = new List<IMU>();//primary motion
            //WitMotion WT901C TTL 9 Axis IMU Sensor Tilt Angle Roll Pitch Yaw + Acceleration + Gyroscope + Magnetometer MPU9250 on PC/Android/MCU
            frontalGyroAccMag.data = "Rx:147 Ry:42 Rz:-23 Ax:1.01 Ay:.01 Az:-.93 Mx:44 My:27.2 Mz:-48";
            spIMUs.Add(frontalGyroAccMag);
            //WitMotion WT901C TTL 9 Axis IMU Sensor Tilt Angle Roll Pitch Yaw + Acceleration + Gyroscope + Magnetometer MPU9250 on PC/Android/MCU
            rearwardGyroAccMag.data = "Rx:142.5 Ry:-46 Rz:-21 Ax:1.02 Ay:0.03 Az:-.922 Mx:47.2 My:28.9 Mz:-47";
            spIMUs.Add(rearwardGyroAccMag);

            //Bosch-BMP085 barometer chipset
            frontalBaro.data = "Bar:.9742";
            spIMUs.Add(frontalBaro);
            //Bosch-BMP085 barometer chipset
            rearwardBaro.data = "Bar:.9781";
            spIMUs.Add(rearwardBaro);

            //Lidar-Lite v3 
            frontalAltimeter.data = "H:1.85";
            spIMUs.Add(frontalAltimeter);
            //Lidar-Lite v3 
            rearwardAltimeter.data = "H:1.91";
            spIMUs.Add(rearwardAltimeter);
            //Lidar-Lite v3 
            starboardAltimeter.data = "H:1.86";
            spIMUs.Add(starboardAltimeter);
            //Lidar-Lite v3 
            portAltimeter.data = "H:1.88";
            spIMUs.Add(portAltimeter);

            //Garmin Striker 4 Fishfinder
            frontalSonar.data = "Sd:15.95 Sr:70.2 Sa:234";
            spIMUs.Add(frontalSonar);

            //Bosch Sensortec BMG250 MEMS
            starboardGyro.data = "Rx:145.9 Ry:104 Rz:-19";
            spIMUs.Add(starboardGyro);
            //Bosch Sensortec BMG250 MEMS
            portGyro.data = "Rx:142 Ry:105.8 Rz:-22.1";
            spIMUs.Add(portGyro);

            List<IMU> ssIMUs = new List<IMU>();//second motion
            //WitMotion WT901C TTL 9 Axis IMU Sensor Tilt Angle Roll Pitch Yaw + Acceleration + Gyroscope + Magnetometer MPU9250 on PC/Android/MCU
            frontalGyroAccMag.data = "Rx:146 Ry:41 Rz:-23.9 Ax:1.01 Ay:.01 Az:-.93 Mx:44 My:29.1 Mz:-49";
            ssIMUs.Add(frontalGyroAccMag);
            //WitMotion WT901C TTL 9 Axis IMU Sensor Tilt Angle Roll Pitch Yaw + Acceleration + Gyroscope + Magnetometer MPU9250 on PC/Android/MCU
            rearwardGyroAccMag.data = "Rx:142 Ry:-48.31 Rz:-21 Ax:1.03 Ay:.01 Az:-.93 Mx:46.9 My:30 Mz:-47.5";
            ssIMUs.Add(rearwardGyroAccMag);

            //Bosch-BMP085 barometer chipset
            frontalBaro.data = "Bar:.978";
            ssIMUs.Add(frontalBaro);
            //Bosch-BMP085 barometer chipset
            rearwardBaro.data = "Bar:.98";
            ssIMUs.Add(rearwardBaro);

            //Lidar-Lite v3 
            frontalAltimeter.data = "H:1.65";
            ssIMUs.Add(frontalAltimeter);
            //Lidar-Lite v3 
            rearwardAltimeter.data = "H:1.70";
            ssIMUs.Add(rearwardAltimeter);
            //Lidar-Lite v3 
            starboardAltimeter.data = "H:1.67";
            ssIMUs.Add(starboardAltimeter);
            //Lidar-Lite v3 
            portAltimeter.data = "H:1.685";
            ssIMUs.Add(portAltimeter);

            //Garmin Striker 4 Fishfinder
            frontalSonar.data = "Sd:15.681 Sr:69 Sa:228";
            ssIMUs.Add(frontalSonar);

            //Bosch Sensortec BMG250 MEMS
            starboardGyro.data = "Rx:145 Ry:103.2 Rz:-20";
            ssIMUs.Add(starboardGyro);
            //Bosch Sensortec BMG250 MEMS
            portGyro.data = "Rx:141.1 Ry:105 Rz:-23";
            ssIMUs.Add(portGyro);

            List<IMU> stIMUs = new List<IMU>();//third motion
            //WitMotion WT901C TTL 9 Axis IMU Sensor Tilt Angle Roll Pitch Yaw + Acceleration + Gyroscope + Magnetometer MPU9250 on PC/Android/MCU
            frontalGyroAccMag.data = "Rx:144.9 Ry:40.1 Rz:-23 Ax:1.01 Ay:.02 Az:-.93 Mx:43.1 My:29.1 Mz:-50.1";
            stIMUs.Add(frontalGyroAccMag);
            //WitMotion WT901C TTL 9 Axis IMU Sensor Tilt Angle Roll Pitch Yaw + Acceleration + Gyroscope + Magnetometer MPU9250 on PC/Android/MCU
            rearwardGyroAccMag.data = "Rx:140.5 Ry:-47 Rz:-20.2 Ax:1.03 Ay:.02 Az:-.92 Mx:46.1 My:30.9 Mz:-48.5";
            stIMUs.Add(rearwardGyroAccMag);

            //Bosch-BMP085 barometer chipset
            frontalBaro.data = "Bar:.981";
            stIMUs.Add(frontalBaro);
            //Bosch-BMP085 barometer chipset
            rearwardBaro.data = "Bar:.991";
            stIMUs.Add(rearwardBaro);

            //Lidar-Lite v3 
            frontalAltimeter.data = "H:1.45";
            stIMUs.Add(frontalAltimeter);
            //Lidar-Lite v3 
            rearwardAltimeter.data = "H:1.50";
            stIMUs.Add(rearwardAltimeter);
            //Lidar-Lite v3 
            starboardAltimeter.data = "H:1.47";
            stIMUs.Add(starboardAltimeter);
            //Lidar-Lite v3 
            portAltimeter.data = "H:1.481";
            stIMUs.Add(portAltimeter);

            //Garmin Striker 4 Fishfinder
            frontalSonar.data = "Sd:17 Sr:65 Sa:212";
            stIMUs.Add(frontalSonar);

            //Bosch Sensortec BMG250 MEMS
            starboardGyro.data = "Rx:143 Ry:165 Rz:-36";
            stIMUs.Add(starboardGyro);
            //Bosch Sensortec BMG250 MEMS
            portGyro.data = "Rx:139 Ry:172 Rz:-39";
            stIMUs.Add(portGyro);

            //-------------------------------------------------------------------------------------
            //-------------------------------------------------------------------------------------
            ///Floating sets
            //Starts with previous stationary sets (IMUs list) and moves from there
            List<IMU> flpIMUs = new List<IMU>();//primary motion
            //WitMotion WT901C TTL 9 Axis IMU Sensor Tilt Angle Roll Pitch Yaw + Acceleration + Gyroscope + Magnetometer MPU9250 on PC/Android/MCU
            frontalGyroAccMag.data = "Rx:146 Ry:41 Rz:-24 Ax:1.01 Ay:.01 Az:-.93 Mx:44 My:29 Mz:-49";
            flpIMUs.Add(frontalGyroAccMag);
            //WitMotion WT901C TTL 9 Axis IMU Sensor Tilt Angle Roll Pitch Yaw + Acceleration + Gyroscope + Magnetometer MPU9250 on PC/Android/MCU
            rearwardGyroAccMag.data = "Rx:143 Ry:-48 Rz:-21 Ax:1.03 Ay:.01 Az:-.93 Mx:47 My:30 Mz:-47.5";
            flpIMUs.Add(rearwardGyroAccMag);

            //Bosch-BMP085 barometer chipset
            frontalBaro.data = "Bar:.978";
            flpIMUs.Add(frontalBaro);
            //Bosch-BMP085 barometer chipset
            rearwardBaro.data = "Bar:.982";
            flpIMUs.Add(rearwardBaro);

            //Lidar-Lite v3 
            frontalAltimeter.data = "H:1.65";
            flpIMUs.Add(frontalAltimeter);
            //Lidar-Lite v3 
            rearwardAltimeter.data = "H:1.71";
            flpIMUs.Add(rearwardAltimeter);
            //Lidar-Lite v3 
            starboardAltimeter.data = "H:1.66";
            flpIMUs.Add(starboardAltimeter);
            //Lidar-Lite v3 
            portAltimeter.data = "H:1.68";
            flpIMUs.Add(portAltimeter);

            //Garmin Striker 4 Fishfinder
            frontalSonar.data = "Sd:15.71 Sr:70 Sa:229";
            flpIMUs.Add(frontalSonar);

            //Bosch Sensortec BMG250 MEMS
            starboardGyro.data = "Rx:145 Ry:103 Rz:-20";
            flpIMUs.Add(starboardGyro);
            //Bosch Sensortec BMG250 MEMS
            portGyro.data = "Rx:141 Ry:105 Rz:-23";
            flpIMUs.Add(portGyro);

            List<IMU> flsIMUs = new List<IMU>();//second motion
            //WitMotion WT901C TTL 9 Axis IMU Sensor Tilt Angle Roll Pitch Yaw + Acceleration + Gyroscope + Magnetometer MPU9250 on PC/Android/MCU
            frontalGyroAccMag.data = "Rx:147 Ry:42 Rz:-23 Ax:1.01 Ay:.01 Az:-.93 Mx:43.5 My:27 Mz:-48.5";
            flsIMUs.Add(frontalGyroAccMag);
            //WitMotion WT901C TTL 9 Axis IMU Sensor Tilt Angle Roll Pitch Yaw + Acceleration + Gyroscope + Magnetometer MPU9250 on PC/Android/MCU
            rearwardGyroAccMag.data = "Rx:142 Ry:-47.2 Rz:-20.1 Ax:1.03 Ay:.01 Az:-.93 Mx:47 My:29 Mz:-46";
            flsIMUs.Add(rearwardGyroAccMag);

            //Bosch-BMP085 barometer chipset
            frontalBaro.data = "Bar:.9761";
            flsIMUs.Add(frontalBaro);
            //Bosch-BMP085 barometer chipset
            rearwardBaro.data = "Bar:.9795";
            flsIMUs.Add(rearwardBaro);

            //Lidar-Lite v3 
            frontalAltimeter.data = "H:1.85";
            flsIMUs.Add(frontalAltimeter);
            //Lidar-Lite v3 
            rearwardAltimeter.data = "H:1.91";
            flsIMUs.Add(rearwardAltimeter);
            //Lidar-Lite v3 
            starboardAltimeter.data = "H:1.86";
            flsIMUs.Add(starboardAltimeter);
            //Lidar-Lite v3 
            portAltimeter.data = "H:1.87";
            flsIMUs.Add(portAltimeter);

            //Garmin Striker 4 Fishfinder
            frontalSonar.data = "Sd:15.95 Sr:70.2 Sa:234";
            flsIMUs.Add(frontalSonar);

            //Bosch Sensortec BMG250 MEMS
            starboardGyro.data = "Rx:145.9 Ry:104 Rz:-19";
            flsIMUs.Add(starboardGyro);
            //Bosch Sensortec BMG250 MEMS
            portGyro.data = "Rx:142 Ry:105.8 Rz:-22.1";
            flsIMUs.Add(portGyro);

            List<IMU> fltIMUs = new List<IMU>();//third motion
            //WitMotion WT901C TTL 9 Axis IMU Sensor Tilt Angle Roll Pitch Yaw + Acceleration + Gyroscope + Magnetometer MPU9250 on PC/Android/MCU
            frontalGyroAccMag.data = "Rx:148.1 Ry:43 Rz:-22.1 Ax:1 Ay:.01 Az:-.921 Mx:43 My:26 Mz:-47.2";
            fltIMUs.Add(frontalGyroAccMag);
            //WitMotion WT901C TTL 9 Axis IMU Sensor Tilt Angle Roll Pitch Yaw + Acceleration + Gyroscope + Magnetometer MPU9250 on PC/Android/MCU
            rearwardGyroAccMag.data = "Rx:143 Ry:-47 Rz:-20.1 Ax:1.02 Ay:0 Az:-.93 Mx:46.5 My:28.1 Mz:-45.9";
            fltIMUs.Add(rearwardGyroAccMag);

            //Bosch-BMP085 barometer chipset
            frontalBaro.data = "Bar:.9738";
            fltIMUs.Add(frontalBaro);
            //Bosch-BMP085 barometer chipset
            rearwardBaro.data = "Bar:.9771";
            fltIMUs.Add(rearwardBaro);

            //Lidar-Lite v3 
            frontalAltimeter.data = "H:2.05";
            fltIMUs.Add(frontalAltimeter);
            //Lidar-Lite v3 
            rearwardAltimeter.data = "H:2.11";
            fltIMUs.Add(rearwardAltimeter);
            //Lidar-Lite v3 
            starboardAltimeter.data = "H:2.06";
            fltIMUs.Add(starboardAltimeter);
            //Lidar-Lite v3 
            portAltimeter.data = "H:2.08";
            fltIMUs.Add(portAltimeter);

            //Garmin Striker 4 Fishfinder
            frontalSonar.data = "Sd:16.1 Sr:70.2 Sa:239";
            fltIMUs.Add(frontalSonar);

            //Bosch Sensortec BMG250 MEMS
            starboardGyro.data = "Rx:147 Ry:105.1 Rz:-18.1";
            fltIMUs.Add(starboardGyro);
            //Bosch Sensortec BMG250 MEMS
            portGyro.data = "Rx:142.9 Ry:106.5 Rz:-21";
            fltIMUs.Add(portGyro);
            //-------------------------------------------------------------------------------------
            string rString;
            if (displayConnected)
                rString = "T::";
            else
                rString = "F::";
            //format of response: ID::Name
            //This is initial access, just getting general IMU information
            int IMUCount = 0;
            //Set type of motion here: Forward, Right, Left, Submerge, Float
            //Planned example, go Forward, Left, Float, Submerge
            string motion = "";
            if (sendCount < 2)
                motion = "Forward";
            else if (sendCount > 2 && sendCount < 5)
                motion = "Left";
            else if (sendCount > 4 && sendCount < 8)
                motion = "Float";
            else if (sendCount > 7)
                motion = "Submerge";

            if (accessType == "Init")
                foreach(IMU imu in IMUs)
                {
                    rString += IMUCount + "::" + imu.IMUType + "::" + imu.IMUName + "::" + imu.Weight + ",,";
                    IMUCount++;
                }
            //format of response: ID::Name
            //This is for getting data from the IMUs. Since this is a fake bot the programmer will choose movement patterns that affect this
            else if (accessType == "Get")
            {
                if(motion == "Forward")
                {
                    if(sendCount == 0)
                    {
                        foreach (IMU imu in fpIMUs)
                        {
                            rString += IMUCount + "::" + imu.IMUType + "::" + imu.IMUName + "::" + imu.Weight + "::" + imu.data + ",,";
                            IMUCount++;
                        }
                    }
                    else if (sendCount == 1)
                    {
                        foreach (IMU imu in fsIMUs)
                        {
                            rString += IMUCount + "::" + imu.IMUType + "::" + imu.IMUName + "::" + imu.Weight + "::" + imu.data + ",,";
                            IMUCount++;
                        }
                    }
                    else if (sendCount == 2)
                    {
                        foreach (IMU imu in ftIMUs)
                        {
                            rString += IMUCount + "::" + imu.IMUType + "::" + imu.IMUName + "::" + imu.Weight + "::" + imu.data + ",,";
                            IMUCount++;
                        }
                    }
                }
                else if(motion == "Left")
                {
                    if (sendCount == 3)
                    {
                        foreach (IMU imu in lpIMUs)
                        {
                            rString += IMUCount + "::" + imu.IMUType + "::" + imu.IMUName + "::" + imu.Weight + "::" + imu.data + ",,";
                            IMUCount++;
                        }
                    }
                    else if (sendCount == 4)
                    {
                        foreach (IMU imu in lsIMUs)
                        {
                            rString += IMUCount + "::" + imu.IMUType + "::" + imu.IMUName + "::" + imu.Weight + "::" + imu.data + ",,";
                            IMUCount++;
                        }
                    }
                }
                //else if(motion == "Right")
                //{
                //    if (sendCount == 5)
                //    {
                //        foreach (IMU imu in rpIMUs)
                //        {
                //            rString += IMUCount + "::" + imu.IMUType + "::" + imu.IMUName + "::" + imu.Weight + "::" + imu.data + ",,";
                //            IMUCount++;
                //        }
                //    }
                //    else if (sendCount == 6)
                //    {
                //        foreach (IMU imu in rsIMUs)
                //        {
                //            rString += IMUCount + "::" + imu.IMUType + "::" + imu.IMUName + "::" + imu.Weight + "::" + imu.data + ",,";
                //            IMUCount++;
                //        }
                //    }
                //}
                else if (motion == "Float")
                {
                    if (sendCount == 5)
                    {
                        foreach (IMU imu in flpIMUs)
                        {
                            rString += IMUCount + "::" + imu.IMUType + "::" + imu.IMUName + "::" + imu.Weight + "::" + imu.data + ",,";
                            IMUCount++;
                        }
                    }
                    else if (sendCount == 6)
                    {
                        foreach (IMU imu in flsIMUs)
                        {
                            rString += IMUCount + "::" + imu.IMUType + "::" + imu.IMUName + "::" + imu.Weight + "::" + imu.data + ",,";
                            IMUCount++;
                        }
                    }
                    else if (sendCount == 7)
                    {
                        foreach (IMU imu in fltIMUs)
                        {
                            rString += IMUCount + "::" + imu.IMUType + "::" + imu.IMUName + "::" + imu.Weight + "::" + imu.data + ",,";
                            IMUCount++;
                        }
                    }
                }
                else if (motion == "Submerge")
                {
                    if (sendCount == 8)
                    {
                        foreach (IMU imu in spIMUs)
                        {
                            rString += IMUCount + "::" + imu.IMUType + "::" + imu.IMUName + "::" + imu.Weight + "::" + imu.data + ",,";
                            IMUCount++;
                        }
                    }
                    else if (sendCount == 9)
                    {
                        foreach (IMU imu in ssIMUs)
                        {
                            rString += IMUCount + "::" + imu.IMUType + "::" + imu.IMUName + "::" + imu.Weight + "::" + imu.data + ",,";
                            IMUCount++;
                        }
                    }
                    else if (sendCount == 10)
                    {
                        foreach (IMU imu in stIMUs)
                        {
                            rString += IMUCount + "::" + imu.IMUType + "::" + imu.IMUName + "::" + imu.Weight + "::" + imu.data + ",,";
                            IMUCount++;
                        }
                    }
                }
                else
                    foreach(IMU imu in IMUs)
                    {
                        rString += IMUCount + "::" + imu.IMUType + "::" + imu.IMUName + "::" + imu.Weight + "::" + imu.data + ",,";
                        IMUCount++;
                    }
            }

            //Null Terminate string
            rString += "\0";

            byte[] response = Encoding.ASCII.GetBytes(rString);
            sendCount += 1;
            return response;
        }

        /// <summary>
        /// Handles the requests from the Senior Project Program by redirecting 
        /// to other functions
        /// </summary>
        /// <param name="tcpClient"></param>
        public static void ClientHandler(TcpClient tcpClient)
        {
            IPEndPoint clientIpa = (IPEndPoint)tcpClient.Client.RemoteEndPoint;
            Console.WriteLine("Connected to client at: {0}:{1}", clientIpa.Address, clientIpa.Port);
            bool connected = true;

            while(connected) 
            {
                NetworkStream stream = tcpClient.GetStream();

                byte[] request = new byte[4096];
                int bytesRead = 0;
                try
                {
                    bytesRead = stream.Read(request, 0, request.Length);
                }
                catch (IOException err) 
                { 
                    Console.WriteLine("IOException on stream.Read(): {0}", err.Message);
                    connected = false;
                }
                catch (Exception err) 
                {
                    Console.WriteLine("An exception occurred on stream.Read(): {0}", err.Message);
                    connected = false;
                }
                bool displayConnected = DisplayConnected(true);
                string requestString = Encoding.ASCII.GetString(request, 0, bytesRead);

                Console.WriteLine(requestString);
                byte[] response = new byte[4096];
                if (requestString == "iRequest")
                    response = InitializationRequestHandler(stream, displayConnected);
                else if (requestString == "gRequest")
                    response = GetRequestHandler(stream, displayConnected);
                else if (requestString == "exiting")
                    connected = false;
                else if (requestString == null)
                    response = Encoding.ASCII.GetBytes("Failed to complete request. Null requestString.");
                else
                    response = Encoding.ASCII.GetBytes("Failed to complete request. Unexpected requestString.");

                Console.WriteLine("Response: {0}", Encoding.UTF8.GetString(response));
                stream.Write(response, 0, response.Length);
                //Set duration of operation here
                if (sendCount == 11)
                    connected = false;
            }
            
            tcpClient.Close();
        }

        /// <summary>
        /// Intended to be used for the first request. Key assumption: IMUs will 
        /// always be accessed in the same order
        /// Output:
        /// IMU Name - Type of unit
        /// IMU ID - Based off of port IMU is connected to, begins at 0
        /// </summary> 
        /// <param name="stream"></param>
        private static byte[] InitializationRequestHandler(NetworkStream stream, bool displayConnected)
        {
            //for each IMU, send IMU Name and IMU ID
            byte[] response = new byte[8192];
            response = AccessIMUs("Init", displayConnected);
            return response;
        }

        /// <summary>
        /// Intended to be used by all noninitial requests. Key assumption: IMUs
        /// will always be accessed in the same order
        /// Output:
        /// IMU ID - Based off of port IMU is connected to, begins at 0
        /// All IMU datapoints (varies by IMU type)
        /// </summary>
        /// <param name="stream"></param>
        private static byte[] GetRequestHandler(NetworkStream stream, bool displayConnected)
        {
            //Get IMUs and their data
            byte[] response = new byte[4096];
            response = AccessIMUs("Get", displayConnected);
            return response;
        }

        /// <summary>
        /// Returns if a display is connected or not. This is hardcoded as this is a emulation of the bot.
        /// Output:
        /// T/F based logically on if a display is connected
        /// </summary>
        /// <param name="displayConnected"></param>
        private static bool DisplayConnected(bool displayConnected)
        {
            if(displayConnected)
                return true;
            else
                return false;
        }
    }
}