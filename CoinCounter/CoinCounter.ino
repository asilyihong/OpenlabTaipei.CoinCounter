#include <DFPlayer_Mini_Mp3.h>
#include <SoftwareSerial.h>

/* settings */
#define ORIG_VOL         30  /* init volume */
#define REDU_VOL         20  /* reduced volume */
#define REDU_INTL       200  /* reduce volume tick */
#define REDU_TIME     10000  /* length to reduce volume of music */
#define RES_INTL         80  /* reduce volume tick when reset */
#define ZERO_TIME     30000  /* length to clear counter */
#define LED_INTERVAL  20000  /* length of music */

/* get coins to launch player */
#define COIN_MAX          5  /* how many coins will work */

/* pin define */
#define COIN_INPUT        2  /* the pin to detect coin input */
#define CUM_RX            8  /* the pin to recv from MiniMp3 */
#define CUM_TX            9  /* the pin to send to MiniMp3 */
#define LED_PIN          12  /* the pin to control relay */

/* configure of LED */
#define LED_ON         HIGH  /* for relay, set LED on */
#define LED_OFF         LOW  /* for relay, set LED off */

/* for check coin signal correct */
#define PASS_MIN_MS      25  /* the minimal millis that passing the tunnel */
#define PASS_MAX_MS     110  /* the maximal millis that passing the tunnel */

static unsigned long startTime = 0;
static unsigned long currTime = 0;
static unsigned long passStartTime = 0;
static unsigned long volumeChangeTime = 0;
static unsigned long passInterval = 0;
static volatile int counter = 0;
static volatile boolean coinPassing = false;
int currVolume = ORIG_VOL;
int ledStatus = LED_OFF;
int coinSig = LOW;
int prevCoinSig = LOW;
int volumeReduIntl = 0;
int volumeReduTick = 0;
int targetVol = 0;

SoftwareSerial cumSerial(CUM_RX, CUM_TX);

void setup()
{
    cumSerial.begin(9600);
    Serial.begin(9600);

    pinMode(LED_PIN, OUTPUT);
    pinMode(COIN_INPUT, INPUT);
    digitalWrite(LED_PIN, ledStatus);
    coinSig = digitalRead(COIN_INPUT);
    prevCoinSig = coinSig;
    currTime = millis();

    mp3_set_serial(cumSerial);
    mp3_single_loop(false);
    mp3_set_reply(false);
}

void loop()
{
    currTime = millis();
    coinSig = digitalRead(COIN_INPUT);
    if (coinSig != prevCoinSig)
    {
        prevCoinSig = coinSig;
        coinPassing = !coinPassing;
        Serial.println(coinPassing);
        if (coinPassing)
        {
            passStartTime = currTime;
            Serial.println(passStartTime);
        }
        else
        {
            passInterval = currTime - passStartTime;
            Serial.println(passInterval);
            if (passInterval > PASS_MIN_MS && passInterval < PASS_MAX_MS)
            {
                counter++;
                Serial.println(counter);
                if (counter == 1)
                {
                    targetVol = 0;
                    volumeReduIntl = RES_INTL;
                    volumeChangeTime = currTime + volumeReduIntl;
                    volumeReduTick = 2;
                }
            }
        }        
    }
    
    if (counter && ledStatus == LED_OFF)
    {
        ledStatus = LED_ON;
        digitalWrite(LED_PIN, ledStatus);
    }
    if (counter == COIN_MAX)
    {
        currVolume = ORIG_VOL;
        counter = 0;
        mp3_set_volume(currVolume);
        delay(40);
        mp3_play(1);
        startTime = currTime;
        volumeChangeTime = startTime + REDU_TIME;
        targetVol = REDU_VOL;
        volumeReduIntl = REDU_INTL;
        volumeReduTick = 1;
    }
    if (currVolume > targetVol && currTime > volumeChangeTime)
    {
        currVolume -= volumeReduTick;
        if (currVolume > 0)
        {
            mp3_set_volume(currVolume);
            volumeChangeTime += volumeReduIntl;
        }
        else
        {
            currVolume = 0;
            targetVol = 0;
            mp3_stop();
        }
    }
    if (currTime - startTime > LED_INTERVAL && ledStatus == LED_ON)
    {
        ledStatus = LED_OFF;
        digitalWrite(LED_PIN, ledStatus);
    }
    if (counter && currTime - passStartTime > ZERO_TIME)
    {
        counter = 0;
    }
}
