// pc-code.cpp

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <array>
#include <chrono>
#include <thread>
#include <visa.h>

// For serial
#include "serialib.h"

constexpr unsigned int BAUDRATE = 38400;

constexpr char CARRIAGE_RETURN = 0x0D;

#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

////////////////////////////////////////////////////////////////////////////////////////////////

bool InstrWrite(char* instrument_address, char* command)
{
    ViSession defaultRM, instr;
    ViStatus status;
    ViUInt32 retCount;

    status = viOpenDefaultRM(&defaultRM);
    if (status != VI_SUCCESS)
    {
        std::cout << "No VISA instrument was opened !" << std::endl;
        return false;
    }
    
    char visa_resource[32];
    sprintf_s(visa_resource, "TCPIP0::%s::INSTR", instrument_address);
    status = viOpen(defaultRM, visa_resource, VI_NULL, VI_NULL, &instr);

    if (status != VI_SUCCESS)
    {
        std::cout << "No VISA instrument was opened !" << std::endl;
        return false;
    }
    
    //write command to the instrument
    status = viWrite(instr, (unsigned char *)command, strlen(command), &retCount);

    //close the instrument
    status = viClose(instr);
    status = viClose(defaultRM);

    return true;
}

bool InstrRead(char* instrument_address, char* result, unsigned int result_size) 
{ 
    ViSession defaultRM, instr;
    ViStatus status;
    ViUInt32 retCount;
    
    //open the VISA instrument
    status = viOpenDefaultRM(&defaultRM);

    if (status != VI_SUCCESS)
    {
        std::cout << "No VISA instrument was opened !" << std::endl;
        return false;
    }

    status = viOpen(defaultRM, instrument_address, VI_NULL, VI_NULL, &instr);

    //read from the instrument
    status = viRead(instr, (unsigned char *)result, result_size, &retCount);

    //close the instrument
    status = viClose(instr);
    status = viClose(defaultRM);
    
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////

unsigned int update_settings(
    unsigned int& start_frequency,
    unsigned int& stop_frequency,
    unsigned int& channel,
    unsigned int& n_steps,
    unsigned int& f_step,
    float& ref_amplitude,
    unsigned int& f_cal,
    char* generator_ip_address,
    unsigned int& com_port
)
{
    unsigned int mode = 255;
    std::cout << "Enter one of the following digit : " << std::endl;
    std::cout << "0: start aquisition" << std::endl;
    std::cout << "1: change start frequency (current: " << start_frequency << " Hz)" << std::endl;
    std::cout << "2: change stop frequency (current: " << stop_frequency << " Hz)" << std::endl;
    std::cout << "3: change output channel (current: " << channel << ")" << std::endl;
    std::cout << "4: change number frequency steps (current: " << n_steps << ")" << std::endl;
    std::cout << "5: change frequency step (current: " << f_step << " Hz)" << std::endl;
    std::cout << "6: change reference amplitude (current: " << ref_amplitude << " V peak)" << std::endl;
    std::cout << "7: change IP address (current: " << generator_ip_address << ")" << std::endl;
    std::cout << "8: change calibration frequency (current: " << f_cal << " Hz)" << std::endl;
    std::cout << "9: start/stop calibration signal generation" << std::endl;
    std::cout << "10: change COM port (current: COM" << com_port  << ")" << std::endl;
    std::cout << "=> "; std::cin >> mode;

    std::string buf;

    switch (mode)
    {
    case 1:
        std::cout << "New start frequency [Hz]: ";
        std::cin >> start_frequency;
        f_step = (unsigned int)std::max(float(stop_frequency - start_frequency) / n_steps, 1.f);
        break;
    case 2:
        std::cout << "New stop frequency [Hz]: ";
        std::cin >> stop_frequency;
        f_step = (unsigned int)std::max(float(stop_frequency - start_frequency) / n_steps, 1.f);
        break;
    case 3:
        do
        {
            std::cout << "Select new channel (1,2): ";
            std::cin >> channel;
        } while (channel != 1 && channel != 2);
        break;
    case 4:
        do
        {
            std::cout << "Enter new number of frequency steps: ";
            std::cin >> n_steps;
        } while (n_steps == 0);
        f_step = (unsigned int)std::max(float(stop_frequency - start_frequency) / n_steps, 1.f);
        break;
    case 5:
        do
        {
            std::cout << "Enter new frequency step: ";
            std::cin >> f_step;
        } while (f_step == 0);
        n_steps = (unsigned int)std::max(float(stop_frequency - start_frequency) / f_step, 1.f);
        break;
    case 6:        
        std::cout << "Enter new output voltage amplitude [V]: ";
        std::cin >> ref_amplitude;
        break;
    case 7:
        std::cout << "Enter new IP address: ";
        std::cin.ignore();
        std::getline(std::cin, buf);
        for (size_t i = 0; i < std::min(buf.size(), 15U); ++i)
        {
            *(generator_ip_address + i) = buf[i];
        }
        generator_ip_address[15] = '\0';
        break;
    case 8:
        std::cout << "Enter new calibration frequency [Hz]: ";
        std::cin >> f_cal;
        break;
    case 10:
        std::cout << "Enter new COM port: ";
        std::cin >> com_port;
        break;
    default:
        break;
    }

    return mode;
}

int main()
{
    std::cout << "FREQUENCY RESPONSE LOGGER" << std::endl;

    unsigned int com_port = 3;
    char serial_port_buf[16];
    sprintf_s(serial_port_buf, "\\\\.\\COM%d", com_port);

    serialib serial;

    // Connection to serial port
    std::cout << "Opening serial port " << serial_port_buf << std::endl;
    char errorOpening = serial.openDevice(serial_port_buf, BAUDRATE);

    if (errorOpening != 1) return errorOpening;
    std::cout << "Successful connection to port " << serial_port_buf << std::endl;

    char serial_buffer[128] = { 0 };

    // Read any long strings that were sent at uC power-up
    while (serial.readString(serial_buffer, '\n', 127, 100) > 0)
    {
        std::cout << "[uC MSG]: " << serial_buffer;
    }

    /////////////////////////////////////////////////////////////////////////

    unsigned int start_frequency = 500, stop_frequency = 3000;
    unsigned int channel = 1;
    unsigned int n_steps = 30, f_step = 83;
    float ref_amplitude = 2.f;
    unsigned int f_cal = 1000;
    bool status = false;
    char generator_ip_address[16] = "169.254.139.187";

    unsigned int mode = 255;
   
    bool cal_mode = false;

    char command[40];

    while (mode != 0)
    {
        mode = update_settings(start_frequency, stop_frequency, channel, n_steps, f_step, ref_amplitude, f_cal, generator_ip_address, com_port);
        if (mode == 9 && !cal_mode)
        {
            std::cout << "Generating calibration signal..." << std::endl;
            sprintf_s(command, ":SOUR%d:APPL:SIN %d,%.1f,0,0", channel, f_cal, ref_amplitude * 2);
            InstrWrite(generator_ip_address, command);

            std::cout << "Turning on output" << std::endl;
            sprintf_s(command, ":OUTP%d ON", channel);
            InstrWrite(generator_ip_address, command);
            cal_mode = true;
        }
        else if (mode == 9 && cal_mode)
        {
            std::cout << "Stopping calibration signal..." << std::endl;
            std::cout << "Turning off output" << std::endl;
            sprintf_s(command, ":OUTP%d OFF", channel);
            InstrWrite(generator_ip_address, command);
            cal_mode = false;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////    

    std::cout << std::endl << "########## Starting aquisition ##########\n" << std::endl;
    
    sprintf_s(command, ":SOUR%d:APPL:SIN %d,%.1f,0,0", channel, start_frequency, ref_amplitude * 2);
    InstrWrite(generator_ip_address, command);
    std::cout << "Turning on output" << std::endl;
    sprintf_s(command, ":OUTP%d ON", channel);
    InstrWrite(generator_ip_address, command);

    using namespace std::chrono_literals;

    std::vector<std::array<int, 2>> response_results{};

    for (unsigned int f = start_frequency; f < stop_frequency; f += f_step)
    {
        memset((void*)serial_buffer, '\0', 128);

        sprintf_s(command, ":SOUR%d:APPL:SIN %d,%.1f,0,0", channel, f, ref_amplitude*2);
        InstrWrite(generator_ip_address, command);
        std::cout << "Test frequency: " << f << " \n";

        // Send sampling period to uC
        int sampling_period_us = (int)(1000000.f / (float)f);
        sprintf_s(serial_buffer, "%d%c", sampling_period_us, CARRIAGE_RETURN); // The CARRIAGE_RETURN indicates end of 'package' to the uC
        serial.writeString(serial_buffer);
        std::cout << "Sent new sampling period: " << sampling_period_us << std::endl;

        // serial_buffer = "xxxxx...\n\0"
        int bytes_read = serial.readString(serial_buffer, '\n', 127, 10000); // Give 10 seconds timeout
        std::cout << "Got uC result !: " << serial_buffer << "\n";
        uint32_t uC_result = atoi(serial_buffer);
        std::array<int, 2> this_result{ (int)f, (int)uC_result };
        response_results.push_back(this_result);
        
        std::this_thread::sleep_for(250ms);
    }

    std::cout << "Turning off output" << std::endl;
    sprintf_s(command, ":OUTP%d OFF", channel);
    InstrWrite(generator_ip_address, command);

    // Close the serial device
    serial.closeDevice();

    // Save to file
    std::ofstream save_file{};
    save_file.open("response.csv");  
    if (save_file.is_open() == false)
    {
        std::cout << "Error opening save file..." << std::endl;
        return 1;
    }
    save_file << "Frequency,Amplitude\n";
    for (auto& elem : response_results)
    {
        save_file << elem[0] << "," << elem[1] << "\n";
    }
    save_file.close();

    std::cout << "Successfully saved to file..." << std::endl;

    system("python.exe ./plotter.py ./response.csv");

    return 0;
}