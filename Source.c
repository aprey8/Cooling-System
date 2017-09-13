#include <msp430.h> 
//ADAM PREY
int LEDS[] = {0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,0x7F,0x67,0x01,0x02,04,0x08,0x10,0x20,0x40,0x80}; //Setting up LEDS for 7 segment

int leftdigit, rightdigit; //initializing digits for 7 segment display

//variables for controlling 7segment display, temperature readings, counting, incrementing, and math.
int flag =0;
int temp = 0, temproom = 0, tempOut;
double tempAVG = 0, temp1;
int count = 0, count2;
int period = 0x0FF;
float D = 0;

//function prototypes
void ConfigureAdc(void);
void getanalogvalues();


int main(void)
{

    WDTCTL = WDTPW | WDTHOLD;       // Stop watchdog timer
    BCSCTL2 |= DIVS_2;              // DIVS_0; DIVS_1; DIVS_2; DIVS_3;
    WDTCTL = WDT_MDLY_0_5;          // WDT_MDLY_32; WDT_MDLY_8; WDT_MDLY_0_5;
    IE1 |= WDTIE;

    //Setting up Pins for LEDS
    P1OUT = 0x00;           //setting all ports to input
    P1REN = 0;              //setting internal resistor
    P1DIR = 0x00;           // port 1 out default 00
    P1DIR |= BIT0|BIT1|BIT2|BIT3|BIT4|BIT5|BIT6;                   // setting output bits for 7 segment
    P2DIR = 0x03;                   // port 2 all inputs, except BIT0 and BIT1


    //Setting up clock
    TA0CTL = TASSEL_2|ID_3|MC_3|TAIE; // This sets the clock register, clock source: smclk at 1 Mhz,
     //signal divider: 8, timer operating mode: up/down so that the clock counts up and then back down from the TACCR0 value, clear timer A, and the timer interrupt enable.
     TA0CCR0 = 62500; //Stores value for the timer to count up/down from, with the current settings
     //these settings generate an interrupt every second.


     //Setting up PWM
     P2DIR    |=  BIT2;             // Set P2.1 and P2.2 as an output-direction
     P2SEL    |=  BIT2;             // Set selection register 1 for timer-function

     TA1CCTL1  = OUTMOD_7;          // Reset/set
     TA1CCR0   = period-1;             // Period
     TA1CCR1   = period*D;              // Duty-cycle
     TA1CTL    = (TASSEL_2 | MC_1); // SMCLK, timer in up-mode

     //setting up switch interrupt
     P2IE = BIT3;//enable interrupt from port P2
     P2IES = BIT3;//interrupt edge select from high to low
     P2IFG = 0x00;//clear the interrupt flag


    __enable_interrupt();       //enabling interrupts
    LPM3; //enter Off mode (low power 3)
    ConfigureAdc();     //running the function to configure the ADC

    // reading the initial room temproom
       __delay_cycles(250);
       getanalogvalues();
       temproom = temp; //setting the ambient temperature
       __delay_cycles(250);



for (;;) //infinite loop
    {

    }

}
    #pragma vector=WDT_VECTOR
__interrupt void WDT(void)
{
    //This executes everytime there is a timer interrupt from WDT
    //The fequency of this interrupt controls the flickering of display
    //The right and left digits are displayed alternatively
    //Note that both digits must be turned off to avoid aliasing

    //Display code for Common-Cathod display
    P1OUT= 0xFF; P2OUT=0xFF;
    __delay_cycles (100);
    if (flag == 0) {P2OUT &= ~BIT0; P1OUT= ~LEDS[leftdigit];  flag=1;}
    else           {P2OUT &= ~BIT1; P1OUT= ~LEDS[rightdigit]; flag=0;}
    __delay_cycles (100);
}

//function to configure ADC
void ConfigureAdc(void)
{
   ADC10CTL1 = INCH_7 | CONSEQ_0| SHS_0|ADC10DIV_0|ADC10SSEL_0;           //P1.7, one channel one conversion, internal ADC clock
   ADC10CTL0 = ADC10SHT_2 | ADC10ON|SREF_0;         //Vcc and Vss references sample for 16 cycles, ADC on
   while (ADC10CTL1 & BUSY); //Wait while ADC is busy
   ADC10AE0 |= (BIT7);  // Enabling a conversion on P1.7
   ADC10CTL0 |= ENC;
}

//function to read analog values
void getanalogvalues()
{
    ADC10CTL0 |= ADC10SC;               // Trigger a conversion
    while (ADC10CTL1 & BUSY);           //Wait while ADC is busy
    temp = ADC10MEM;                    //assigns ADC value to temp.
}

// ADC interrupt
#pragma vector=ADC10_VECTOR
__interrupt void ADC10_ISR(void)
{
    __bic_SR_register_on_exit(CPUOFF);
}

//Timer interrupt
#pragma vector=TIMER0_A1_VECTOR
__interrupt void Timer_A(void)
{
     switch(TAIV)
     {
         case 0x02: break;                  // Capture/Compare 1 TACCR1 CCFIG interrupt flag
         case 0x04: break;                  // Capture/Compare 2 TACCR2 CCFIG interrupt flag
         case 0x0A: count++, count2++;           //0A us the timer overflow and is the TAIFG interrupt flag
     break;
     }
         //displaying last 2 digits of Z number for 10 seconds
         if(count2<=10)
         {
             leftdigit = 6;
             rightdigit = 2;
         }
         if (count <=5 ) //To get readings every 5 seconds
         {
             getanalogvalues();     //get values
             temp1 = temp1 + temp;  // storing the temp into a variable for averaging
         }
         if (count == 5)
         {
             tempAVG = (temp1 / 5);     //calculating the average
             temp1 = 0;                 //resetting the storage variable
             tempOut = ((10 * tempAVG)+500)/100;        //an attempt to convert ADC reading to degrees C, found in manual, was off by a factor of 100, so I divide by 100
             tempOut = (tempOut * 1.8)+29; //convert to C to F
             count = 0; //reset my count
             leftdigit = tempOut / 10;  //assigning the ones place from the temp to the 7 segment
             rightdigit = tempOut % 10;     //assingin the tens place from temp to the 7 segment
         }
         if(tempOut <78)        //This block of code changes the multiplier for the PWM over the temp range.
             {
             D = 0;
             }else if(tempOut == 78){
             D = .081;
             }else if (tempOut == 79){
             D = .162;
             }else if (tempOut == 80){
             D = .243;
             }else if (tempOut == 81){
             D = .324;
             }else if (tempOut == 82){
             D = .405;
             }else if (tempOut == 83){
             D = .486;
             }else if (tempOut == 84){
             D = .567;
             }else if (tempOut == 85){
             D = .648;
             }else if (tempOut == 86){
             D = .729;
             }else if (tempOut == 87){
             D = .81;
             }
             else if (tempOut == 88){
             D = .95;
             }
         TA1CCR1   = period*D; //Calculates and sets the new duty cycle
}
#pragma vector=PORT2_VECTOR
//define the interrupt vector
__interrupt void PORT2_ISR(void)
{
    // Interrupt Service Routine
    LPM3_EXIT; //exit LPM on switch
     P2IFG = 0x00; //clear the interrupt flag
}
