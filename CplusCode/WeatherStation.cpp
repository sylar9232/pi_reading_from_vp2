/*
*
*  AUTHOR : DENIS PRSA
*
*
*/

#include <iostream>
#include <stdio.h>      // standard input / output functions
#include <stdlib.h>
#include <string.h>     // string function definitions
#include <unistd.h>     // UNIX standard function definitions
#include <fcntl.h>      // File control definitions
#include <errno.h>      // Error number definitions
#include <termios.h>    // POSIX terminal control definitions
#include <getopt.h>     // OPTIONS IN TERMINAL
#include <vector>
#include <ctype.h>
#include <stdint.h>
#include <sstream>
#include <complex>
#include "WeatherStation.h"
#include "main.h"


using namespace std;

//
// FOR MORE DETAILS SEE : http://www.davisnet.com/support/weather/download/VantageSerialProtocolDocs_v261.pdf
//


// CONSTRUCTOR
WeatherStation::WeatherStation(string path){
    this->path = path.c_str();
    this->yDelay = 10;
    

}



// --------------------------------------------------------
// FUNCTION MENU WITH OPTIONS
// --------------------------------------------------------
void WeatherStation::menu(int argc, char *argv[]){
    
    // VARIABLES FOR READING ARGUMENTS FROM CONSOLE
    extern char *optarg;
    extern int optind, opterr, optopt;
    
    int c;
    
    static struct option longopts[] = {
        { "archive",    required_argument,      0,      'a' },
        { NULL,         0,                      NULL,   0 }
    };
    
    
    if(argc == 1){
        this->showMenu();
        exit(2);
    }
    while ((c = getopt_long(argc, argv, "a:", longopts, NULL )) != EOF) {
        switch (c) {
            
            case 'a':
                
                // OPENING SERIAL PORT
                this->OpenSerialPort();
                // WAKING UP WEATHER STATION
                
                if(this->WakeUpStation() != -1){
                    cout << "Error while waking up weather station! Check connection." << endl;
                    exit(2);
                }
                // SEND COMMAND TO READ OUT ARHIVE DATA
                if(write(this->fd, "DMPAFT\n", 7) != 7){
                    cout << "Error while writing to serial port " << endl;
                    exit(2);
                }
                if(!checkACK()){
                    exit(2);
                }
                
                
                // IF NO ERROR THEN GET TIME FROM CONSOLE, OR READ FROM FILE. EXIT IF NO DATETIME PASSED.
                const char *dateTime = this->getDateTime(optarg);
                
                // WRITE DATE TIME AND LOW AND HIGH BITS
                if(write(this->fd, dateTime, 6) != 6)
                {
                    cout << "Error while writing to serial port " << endl;
                    exit(2);
                }
                tcdrain(this->fd);
                
                char ch ;
                this->ReadNextChar(&ch);
                cout << "1 " << (int)ch << endl;
                this->ReadNextChar(&ch);
                cout << "2 " << (int)ch << endl;
                this->ReadNextChar(&ch);
                cout << "3 " << (int)ch << endl;
                this->ReadNextChar(&ch);
                cout << "4 " << (int)ch << endl;
                this->ReadNextChar(&ch);
                cout << "5 " << (int)ch << endl;
                this->ReadNextChar(&ch);
                cout << "6 " << (int)ch << endl;
                this->ReadNextChar(&ch);
                cout << "7 " << (int)ch << endl;
                
                
                
                static char ACKS[1];
                ACKS[0] = 0x06;
                
                if(write(this->fd, &ACKS, strlen(ACKS) ) != strlen(ACKS)){
                    cout << " Napakajdfjoajdi " << endl;
                }
                tcdrain(this->fd);
                
                int nCnt;
                static char szSerBuffer[4200];
                nCnt = ReadToBuffer(szSerBuffer, sizeof(szSerBuffer));
                cout << "vproweather: mora biti nic " << nCnt << endl;
                if(nCnt != 267 ){
                    fprintf(stderr, "Napaka \n");
                    break;
                }

                /*
                if(!checkACK()){
                    exit(2);
                }
                
                int nCnt;
                static char szSerBuffer[4200];
                
                nCnt = this->ReadToBuffer(szSerBuffer, sizeof(szSerBuffer));
                cout << "Dobljenih paketov: " << nCnt << endl;
                
                if((nCnt = this->CheckCRC(6, szSerBuffer))) {
                    cout << "crc koda : " << nCnt << endl;
                }
                
                char pa2 = szSerBuffer[0];
                char pa1 = szSerBuffer[1];
                cout << " pa1 " << (int)pa1 << " pa2 " << (int)pa2 << endl;
                int number = pa2 | pa1 << 8;
                
                char za2 = szSerBuffer[2];
                char za1 = szSerBuffer[3];
                cout << " za1 " << (int)pa1 << " za2 " << (int)za2 << endl;
                int number2 = za2 | za1 << 8;
                
                cout << "strani " << number << endl;
                */
                
                break;
        }
    }
}


// --------------------------------------------------------
// FUNCTION THAT CHECKS FOR ACK
// --------------------------------------------------------
bool WeatherStation::checkACK(){
    char ch = 0x00;
    this->ReadNextChar(&ch);
    cout << ch << endl;
    if(ch != 0x06){
        cout << "Error ! No ACK recevied." << endl;
        return false;
    }
    return true;
}


// --------------------------------------------------------
// FUNCTION THAT GETS TIME FROM FILE OR CONSOLE
// --------------------------------------------------------
char *WeatherStation::getDateTime(char *_string){
    
    // IF NO ERROR THEN GET TIME FROM CONSOLE, OR READ FROM FILE. EXIT IF NO DATETIME PASSED.
    int counter = 3;
    int year;
    int month;
    int day;
    int hour;
    int minute;
    char *datah = new char[6];
    int16_t i;
    
    if(_string[0] == 'D' && _string[1] == ':' && _string[2] == ':'){
        
        while (_string[counter] != '\0') {
            if(_string[counter] == 'y'){
                year = this->getNumberFromChar(_string, counter);
            } else if (optarg[counter] == 'm'){
                month = this->getNumberFromChar(_string, counter);
            } else if (optarg[counter] == 'd'){
                day = this->getNumberFromChar(_string, counter);
            } else if (optarg[counter] == 'h'){
                hour = this->getNumberFromChar(_string, counter);
            } else if (optarg[counter] == 'i'){
                minute = this->getNumberFromChar(_string, counter);
            }
            counter ++;
        }
        
        int vantageDateStamp = day + month*32 + (year-2000)*512;
        int vantageTimeStamp = (100*hour + minute);
        
        char b1 = (vantageDateStamp >> 8) & 0xFF;
        char b2 = vantageDateStamp & 0xFF;
        
        char c1 = (vantageTimeStamp >> 8) & 0xFF;
        char c2 = vantageTimeStamp & 0xFF;
        
        
        datah[0] = b2;
        datah[1] = b1;
        datah[2] = c2;
        datah[3] = c1;
        i = this->CheckCRC(4, datah);
        datah[4] = HIBYTE(i);
        datah[5] = LOBYTE(i);
        return datah;

    } else if (_string[0] == 'F' && _string[1] == ':' && _string[2] == ':'){
        
    } else {
        cout << "No datetime or file passed. Exiting!" << endl;
        exit(2);
    }
    
    return new char[1];
}




// --------------------------------------------------------
// FUNCTION THAT CONVERTS NUMBER INT FROM CHAR
// --------------------------------------------------------
int WeatherStation::getNumberFromChar(char *_string, int &row){
    
    if( !isdigit(_string[row+1]) ){
        cout << "Invalid datetime!" << endl;
        exit(2);
    }
    string _number = "";
    do {
        _number.push_back(_string[row+1]);
        row ++;
    } while(isdigit(_string[row+1]));
    
    int result;
    stringstream(_number) >> result;
    return result;
}

// --------------------------------------------------------
// FUNCTION THAT SHOWS MENU WITH OPTIONS
// --------------------------------------------------------
void WeatherStation::showMenu(){
    cout << endl;
    cout << "------------------------------------------------------------------------------------" << endl;
    cout << "| Sowtware that reads data from Davis Vantage Pro 2 with Raspbery Pi" << endl;
    cout << "| Author : Denis Prsa, http:://vremezizki.si/" << endl;
    cout << "|" << endl;
    cout << "| Parameters: " << endl;
    cout << "|  --a  required parameter : date ( D::y2016m5d5h10i20) OR ( F::filename.txt ) " << endl;
    cout << "|       ( read data from archive if new data stored )" << endl;
    cout << "|       ( y = year, m = month, d = day, h = hour, i = minute )" << endl;
    cout << "------------------------------------------------------------------------------------" << endl;
    cout << endl;
}

// --------------------------------------------------------
// FUNCTION FOR OPENING PORT
// --------------------------------------------------------
void WeatherStation::OpenSerialPort(){

    //open port
    this->fd = open(this->path, O_RDWR | O_NOCTTY );

    //check if opening failed
    if(this->fd == -1){
        cout << "error while opening port : Unable to open  " <<  path << "."<< endl;
        return;
    }

    // save previous state
    tcgetattr(this->fd, &this->oldsio);
    // set new state
    bzero(&this->newsio, sizeof(this->newsio));

    // set setings for opening serial port
    this->newsio.c_cflag = CRTSCTS | CS8 | CLOCAL | CREAD;
    this->newsio.c_iflag = IGNBRK | IGNPAR;
    this->newsio.c_oflag = 0;
    this->newsio.c_lflag = 0;
    this->newsio.c_cc[VTIME]    = this->yDelay; /* timeout in 1/10 sec intervals */
    this->newsio.c_cc[VMIN]     = 0;      /* block until char or timeout */
    cfsetospeed (&this->newsio, B19200);
    cfsetispeed (&this->newsio, B19200);

    if(tcsetattr(this->fd, TCSAFLUSH, &this->newsio)) {
        cout << "error while opening port : Problem configuring serial device, check device name." << endl;
        exit(2);
    }

    if(tcflush(this->fd, TCIOFLUSH)) {
        cout << "error while opening port : Problem flushing serial device, check device name." << endl;
        exit(2);
    }

}


// --------------------------------------------------------
// FUNCTION FOR WAKING UP STATION
// --------------------------------------------------------
int WeatherStation::WakeUpStation(){
    char ch;
    int i;

    // FROM DOC : If the console has not woken up after 3 attempts, then signal a connection error
    for(i = 0; i < 3; i++)
    {
        // FROM DOC : Send a Line Feed character, ‘\n’ (decimal 10, hex 0x0A).
        ch = '\n';
        if(write(this->fd, &ch, 1) != 1)
        {
            cout <<  "reading from weather station: Problem reading serial device (one char).";
            exit(2);
        }
        
        // CALL FUNCTION TO READ CHAR FROM SERIAL PORT
        // FROM DOC : Listen for a returned response of Line Feed and Carriage Return characters, (‘\n\r’).
        if(this->ReadNextChar(&ch))
        {
            // IF THERE IS RESPONSE, IT'S GONNA GE  ‘\n\r’ SO NO NEED TO CLEAR '\r'
            this->ReadNextChar(&ch);
            cout << "Weather station woke up after " << i+1 << " retries." << endl;
            return -1;
        }
    }

    return 0;
}

// --------------------------------------------------------
// FUNCTION FOR READING NEXT CHAR FROM WEATHER STATION
// --------------------------------------------------------
int WeatherStation::ReadNextChar( char *pChar ){
    int result;

    result = read(this->fd, pChar, 1);
    if(result == -1) {
        cout <<  "reading from weather station: Problem reading serial device (one char).";
        exit(2);
    }
    // RETURN 1 CHARACTER SO == TRUE
    return result;
}


// --------------------------------------------------------
// FUNCTION THAT CHECKS FOR CRC CODE
// --------------------------------------------------------
int WeatherStation::CheckCRC( int _num, char *_data ){
    int i;
    uint16_t wCRC = 0;
    uint8_t y;
    static uint16_t crc_table[256] = {
        0x0,     0x1021,  0x2042,  0x3063,  0x4084,  0x50a5,  0x60c6,  0x70e7,
        0x8108,  0x9129,  0xa14a,  0xb16b,  0xc18c,  0xd1ad,  0xe1ce,  0xf1ef,
        0x1231,  0x210,   0x3273,  0x2252,  0x52b5,  0x4294,  0x72f7,  0x62d6,
        0x9339,  0x8318,  0xb37b,  0xa35a,  0xd3bd,  0xc39c,  0xf3ff,  0xe3de,
        0x2462,  0x3443,  0x420,   0x1401,  0x64e6,  0x74c7,  0x44a4,  0x5485,
        0xa56a,  0xb54b,  0x8528,  0x9509,  0xe5ee,  0xf5cf,  0xc5ac,  0xd58d,
        0x3653,  0x2672,  0x1611,  0x630,   0x76d7,  0x66f6,  0x5695,  0x46b4,
        0xb75b,  0xa77a,  0x9719,  0x8738,  0xf7df,  0xe7fe,  0xd79d,  0xc7bc,
        0x48c4,  0x58e5,  0x6886,  0x78a7,  0x840,   0x1861,  0x2802,  0x3823,
        0xc9cc,  0xd9ed,  0xe98e,  0xf9af,  0x8948,  0x9969,  0xa90a,  0xb92b,
        0x5af5,  0x4ad4,  0x7ab7,  0x6a96,  0x1a71,  0xa50,   0x3a33,  0x2a12,
        0xdbfd,  0xcbdc,  0xfbbf,  0xeb9e,  0x9b79,  0x8b58,  0xbb3b,  0xab1a,
        0x6ca6,  0x7c87,  0x4ce4,  0x5cc5,  0x2c22,  0x3c03,  0xc60,   0x1c41,
        0xedae,  0xfd8f,  0xcdec,  0xddcd,  0xad2a,  0xbd0b,  0x8d68,  0x9d49,
        0x7e97,  0x6eb6,  0x5ed5,  0x4ef4,  0x3e13,  0x2e32,  0x1e51,  0xe70,
        0xff9f,  0xefbe,  0xdfdd,  0xcffc,  0xbf1b,  0xaf3a,  0x9f59,  0x8f78,
        0x9188,  0x81a9,  0xb1ca,  0xa1eb,  0xd10c,  0xc12d,  0xf14e,  0xe16f,
        0x1080,  0xa1,    0x30c2,  0x20e3,  0x5004,  0x4025,  0x7046,  0x6067,
        0x83b9,  0x9398,  0xa3fb,  0xb3da,  0xc33d,  0xd31c,  0xe37f,  0xf35e,
        0x2b1,   0x1290,  0x22f3,  0x32d2,  0x4235,  0x5214,  0x6277,  0x7256,
        0xb5ea,  0xa5cb,  0x95a8,  0x8589,  0xf56e,  0xe54f,  0xd52c,  0xc50d,
        0x34e2,  0x24c3,  0x14a0,  0x481,   0x7466,  0x6447,  0x5424,  0x4405,
        0xa7db,  0xb7fa,  0x8799,  0x97b8,  0xe75f,  0xf77e,  0xc71d,  0xd73c,
        0x26d3,  0x36f2,  0x691,   0x16b0,  0x6657,  0x7676,  0x4615,  0x5634,
        0xd94c,  0xc96d,  0xf90e,  0xe92f,  0x99c8,  0x89e9,  0xb98a,  0xa9ab,
        0x5844,  0x4865,  0x7806,  0x6827,  0x18c0,  0x8e1,   0x3882,  0x28a3,
        0xcb7d,  0xdb5c,  0xeb3f,  0xfb1e,  0x8bf9,  0x9bd8,  0xabbb,  0xbb9a,
        0x4a75,  0x5a54,  0x6a37,  0x7a16,  0xaf1,   0x1ad0,  0x2ab3,  0x3a92,
        0xfd2e,  0xed0f,  0xdd6c,  0xcd4d,  0xbdaa,  0xad8b,  0x9de8,  0x8dc9,
        0x7c26,  0x6c07,  0x5c64,  0x4c45,  0x3ca2,  0x2c83,  0x1ce0,  0xcc1,
        0xef1f,  0xff3e,  0xcf5d,  0xdf7c,  0xaf9b,  0xbfba,  0x8fd9,  0x9ff8,
        0x6e17,  0x7e36,  0x4e55,  0x5e74,  0x2e93,  0x3eb2,  0xed1,   0x1ef0,
    };

    
    for( i = 0; i < _num; i++) {
        y = *(_data)++;
        wCRC = crc_table[(wCRC >> 8) ^ y] ^ (wCRC << 8);
    }
    
    return wCRC;
}



// --------------------------------------------------------
// FUNCTION THAT CHECKS FOR CRC CODE
// --------------------------------------------------------
int WeatherStation::ReadToBuffer( char *pszBuffer, int nBufSize)
{
    int nPos = 0;               /* current character position */
    char *pBuf = pszBuffer;
    
    while(nPos < nBufSize) {
        if(!ReadNextChar(pBuf++))
            return nPos;        /* no character available */
        ++nPos;
    }
    return -1;                  /* buffer overflow */
}


