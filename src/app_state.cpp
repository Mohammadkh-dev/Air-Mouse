#include "app_state.h"

bool micOn = false;
String btName = "Bluetooth";
int batteryLevel = 0;

int currentScreen = 0;
int menuIndex = 0;
const char* menuItems[] = {"Stopwatch", "Timer", "Set Clock", "About"};

unsigned long clockTime = 0;
unsigned long lastUpdateMs = 0;
unsigned long lastOledRefreshMs = 0;
bool noSleep = true;
bool initialSleepRender = true;
bool blockLeftClick = false;
bool blockBothClick = false;

unsigned long swStartTime = 0;
unsigned long swElapsed = 0;
unsigned long swPauseOffset = 0;
bool swRunning = false;
int swState = 0;

unsigned long tmSetupSeconds = 0;
unsigned long tmStartTime = 0;
unsigned long tmElapsed = 0;
unsigned long tmPauseOffset = 0;
bool tmRunning = false;
int tmState = 0;
int tmSetupField = 0;
unsigned long tmSetH = 0;
unsigned long tmSetM = 0;
unsigned long tmSetS = 0;

int scField = 0;
unsigned long scSetH = 0;
unsigned long scSetM = 0;
unsigned long scSetS = 0;

bool mic = true;
volatile bool mode = true;
unsigned int dongleCheck = 0;
uint8_t receiverAddress[] = {0xB8, 0xF8, 0x62, 0x62, 0xB0, 0xA4};
volatile int sendFailureCounter = 0;
DonglePacket dataPacket;
int32_t rawBuffer[AUDIO_SAMPLES];

NimBLEServer* pServer = nullptr;
NimBLEHIDDevice* pHid = nullptr;
NimBLECharacteristic* pMouseInput = nullptr;
NimBLECharacteristic* pKeyInput = nullptr;
bool deviceConnected = false;
bool bleAdvertisingActive = false;
unsigned long winKeyTimer = 0;
int winKeyState = 0;

int16_t accumMouseX = 0;
int16_t accumMouseY = 0;
bool leftClicked = false;
bool taskViewTriggered = false;

TaskHandle_t audioTaskHandle = NULL;
