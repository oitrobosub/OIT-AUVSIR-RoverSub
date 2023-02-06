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
        private struct IMU
        {
            public int IMUID;
            public string IMUType;
            public string IMUName;
            public float Weight;
            public string ? data;
        }
        private IMU frontalGyroAccMag;
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
            //CONTINUED: where SIDEOFBOT is either F/R/P/S for Frontal/Rear/Port/Starboard
            List<IMU> IMUs = new List<IMU>();

            //-------------------------------------------------------------------------------------
            ///BOTH OF THESE ARE TAKEN AT THE SAME INSTANT WHILE ACCELERATING FORWARD
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
            //-------------------------------------------------------------------------------------

            IMUs.Add(rearwardGyroAccMag);
            string rString;
            if (displayConnected)
                rString = "T::";
            else
                rString = "F::";
            //format of response: ID::Name
            //This is initial access, just getting general IMU information
            int IMUCount = 0;
            if (accessType == "Init")
                foreach(IMU imu in IMUs)
                {
                    rString += IMUCount + "::" + imu.IMUType + "::" + imu.IMUName + "::" + imu.Weight + ",,";
                    IMUCount++;
                }
            //format of response: ID::Name
            //This is for getting data from the IMUs. Since this is a fake bot it will randomly choose movement patterns here
            else if (accessType == "Get")
            {
                foreach(IMU imu in IMUs)
                {
                    rString += IMUCount + "::" + imu.IMUType + "::" + imu.IMUName + "::" + imu.Weight + "::" + imu.data + ",,";
                    IMUCount++;
                }
            }

            //Null Terminate string
            rString += "\0";

            byte[] response = Encoding.ASCII.GetBytes(rString);
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
                //TODO: This is where FakeBot sends fake IMU data to the program
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