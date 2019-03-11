/***************************头文件***********************/ 
#include <reg52.h> 
#include <intrins.h> 
/************************数据类型宏定义******************/ 
#define uchar unsigned char 
#define uint unsigned int 
/***********************使能、禁止宏定义*****************/ 
#define Enable(); EA=1; 
#define Disable();EA=0; 
/**********************电机控制宏定义********************/ 
#define ZHENGZHUAN 0 
#define STOP 1 
#define FANZHUAN 2 
#define ZhengZhuan(); PIN_Motor_A = 0; PIN_Motor_B = 1; 
#define Stop(); PIN_Motor_A = 1; PIN_Motor_B = 1; 
#define FanZhuan(); PIN_Motor_A = 1; PIN_Motor_B = 0; 
/*******************暂停、继续控制宏定义*****************/ 
#define ZanTing(); ET0 = 0; ET1 = 0; Stop(); 
#define JiXu(); ET0 = 1; ET1 = 1; 
#define flag_Ok  (flag_XiDi||flag_TuoShui) 
/*************************管脚定义***********************/ 
#define lcd P0     //液晶数据口

sbit PIN_JinShui = P2^2; //进水
sbit PIN_PaiShui = P2^3; //排水
sbit PIN_Motor_A = P2^4; //电机脚A 
sbit PIN_Motor_B = P2^5; //电机脚B 
sbit K_SEL_ChengXu = P1^5; //选择程序键 
sbit qiangruo = P1^6; //水量选择 
sbit shuiwei1 = P3^6; // 水位监测
sbit p32 = P3^2; //   启动/暂停
sbit rs=P1^1;	   //液晶控制引脚
sbit en=P1^2;
sbit SPK = P2^0; //报警喇叭
/*************************数据定义***********************/ 
uchar flag_SEL_ChengXu; //默认为标准程序， 
 
bit flag_Run; //运行标志，1为运行 
bit flag_XiDi; //置洗涤标志 
bit flag_TuoShui; //脱水标志
bit flag_PiaoXi;  //漂洗标志  
bit flag_SEl_QiangRuo; //默认为强洗，1为弱洗 

uchar _50ms; //每50ms加一次的变量 
uint s,s1; //秒 
uchar k;
int counter1=0,counter2=0,counter3=0;  //电机转速控制
uint T_S; //定时总时间  
int fen,miao;//剩余时间
uint t1,t2,t3;	   //洗涤、漂洗、甩干时间
uchar a=15;  //占空比
uchar mol=0;  //手动模式选择
uint count;
/**************************数组定义********************/
uchar code table0[]="State:          ";

uchar table7[]="00:00";


/*************************延时程序*********************/ 
void Delay_10ms(uint T1) 
{ 
	uint t1,t2; 
	for(t1=0;t1<T1;t1++) 
	for(t2=0;t2<1250;t2++);//10ms 
} 


void delay(uint count)		 //1ms延时
{
   uint x,y;
   for(x=0;x<count;x++)
   for(y=0;y<120;y++);
}
/*************************液晶驱动程序*************************/
void w_cmd(uchar com)		  //lcd1602写命令
{
  rs=0;
  lcd=com;
  en=1;
  delay(5);
  en=0;
}

void w_data(uchar dat)		  //lcd1602读数据
{
  rs=1;
  lcd=dat;
  en=1;
  delay(5);
  en=0;
}

void w_str(uchar *s)       //lcd1602写字符串
{
    while(*s)  w_data(*s++);
}

void lcd_int()		     //lcd1602初始化
{
  en=0;
  w_cmd(0x38);
  w_cmd(0x0c);
  w_cmd(0x06);
  w_cmd(0x01);
  w_cmd(0x80+0x00);
  w_str(table0);
  //w_cmd(0xc0+0x00);
 // w_str(table1);
}


/********************中断初始化程序********************/ 
void ExInt_Init(void) 
{ 
	IT0 = 1; 
	EX0 = 1; 
	IT1 = 1; 
	EX1 = 1; 
} 
/*******************定时器0初始化程序******************/ 
void Timer0_Init(void) //其中没ET0是为以后控制暂停用的 
{ 
	TMOD = TMOD | 0X01; //定时方式1 
	TH0 = 0X3C; //50ms 
	TL0 = 0XB0; 
	TR0 = 1; 
	_50ms = 0; 
	s = 0; 
} 
/*******************定时器1初始化程序******************/ 
void Timer1_Init(void) // 
{ 
	TMOD=TMOD|0x10;
	
	TH1 = ( 65535 - 500 ) / 256;   //1ms 
	TL1 = ( 65535 - 500 ) % 256; 
	//ET1=1;	
	//EA=1;
	TR1=1;	

	
}


/*******外部中断0程序为运行或者暂停***********/ 
void int0(void) interrupt 0 
{ 
	 if(!p32) 
	 { 
	   Delay_10ms(1); //延时10ms左右，去抖动 
	   if(!p32) 
	   { 
		 	if(!flag_Run)
			{ 
		     	flag_Run = 1; //置运行标志
				
			} 
		    
		    else if(flag_Ok) 
		    { 
			      static bit flag_ZanTing; 
			      flag_ZanTing = ~flag_ZanTing; 
			      if(flag_ZanTing) 
			      {
				  	ZanTing();
				   }
					else 
					{JiXu();} 
		    } 
	    } 
	  }
 } 

/*******************定时器0中断程序********************/ 
void Timer0(void) interrupt 1 
{ 
	 TR0 = 0; //停止计数 
	 TH0 = 0X3C; //重装定时器值 
	 TL0 = 0XB0; 
	 TR0 = 1; 
	 _50ms++; 
	 if(_50ms == 20) //1s到 
	 { 
		   _50ms = 0; 
		   
		  s++;
		  if(s == T_S) //定时到 
		  {
			 s = 0; 			 
   			 flag_XiDi = 0; //清洗涤标志 
 			 flag_TuoShui= 0; //清脱水标志
			 flag_PiaoXi=0; //清漂洗标志
			 Stop();
			 ET1=0;
		  } 
		  else
		  { 
			  	ET1=1; //电机控制程序 
				
			   
			   
			   if(miao==0)
			   {
			   		miao=60;
					fen--;
					if(fen==0)
						fen=0;
			   }
			   miao--;
			   
		  }
		
	 } 
} 

/*******************定时器1中断程序********************/ 
void Timer1 ( void ) interrupt 3  //定时器1中断函数 0.5ms 
{ 
	TH1 = ( 65535 - 500) / 256;  
	TL1 = ( 65535 - 500 ) % 256;
	if(flag_XiDi||flag_PiaoXi)		 
	{
			count++;
			if(count<16000)
			{
				counter1++;	  //d电机控制计数
				if(counter1<=a)
				{
					ZhengZhuan();		 				
				}
				else if(counter1>a)
				{
					Stop();
				}
				if(counter1==100) counter1=0;
			}
			else if(count<20000)
			{
				 Stop();
			}
			else if(count<36000)
			{
				counter3++;	  //d电机控制计数
				if(counter3<=a)
				{
					FanZhuan();		 				
				}
				else if(counter3>a)
				{
					Stop();
				}
				if(counter3==100) counter3=0;
			}
			else
			{
				count=0;
			}
	
	}
	 

	if(flag_TuoShui)
	{
		counter2++;	  //d电机控制计数
		if(counter2<=40)
		{
			ZhengZhuan();		 				
		}
		else if(counter2>40)
		{
			Stop();
		}
		if(counter2==100) counter2=0;
	}
}


/******************进水排水**********************/
void JinShui()
{
	PIN_JinShui = 0; //进水控制 
    PIN_PaiShui = 1;
}
void PaiShui()
{
	PIN_JinShui = 1; //pai水控制 
    PIN_PaiShui = 0;
}
/**********************洗涤程序***********************/ 
void XiDi(void) 
{ 

	 	 	
		JinShui();	   //进水
		while(shuiwei1)
		{	 w_cmd(0x80+6);	
	        w_str(" JinShui"); 
		}									
		 PIN_JinShui = 1; //关进水阀门
		 	 w_cmd(0x80+6);	
	      w_str("  XiDi    "); 
		 // Timer0_Init(); 
		 ET0=1;
		  T_S =t1;
	     fen=t1/60;miao=t1%60; 
	     
		 flag_XiDi=1;
		  while(flag_XiDi)
		 {	 
			 table7[0]=fen/10+0x30;
			 table7[1]=fen%10+0x30;
			 table7[3]=miao/10+0x30;
			 table7[4]=miao%10+0x30;
			 w_cmd(0xc0+7);	
			 w_str(table7);
			 
		}
		 
		ET0=0;
		w_cmd(0xc0+7);	
		 w_str("00:00");
		 
			PaiShui();		//排水
			while(!shuiwei1)
			{
				w_cmd(0x80+7);	
        	    w_str(" PaiShui");
			}
		
		 PIN_PaiShui = 1; //关排水阀门	 

	
} 
/*****************漂洗***************************/
void PiaoXi()
{
	
		 	  
	    JinShui();	   //进水
		while(shuiwei1)
		{	 w_cmd(0x80+6);	
	        w_str(" JinShui"); 
		}		
		PIN_JinShui = 1; //关进水阀门
			w_cmd(0x80+7);	
        	w_str(" PiaoXi");	
		 ET0=1;
		 T_S =t2;
		 fen=t2/60;miao=t2%60; 	     
		 flag_PiaoXi=1;
		 
		  while(flag_PiaoXi)
		 {	 
			 table7[0]=fen/10+0x30;		//显示剩余时间
			 table7[1]=fen%10+0x30;
			 table7[3]=miao/10+0x30;
			 table7[4]=miao%10+0x30;
			 w_cmd(0xc0+7);	
			 w_str(table7);			 
		 }
		 ET0=0;
		 w_cmd(0xc0+7);	
		 w_str("00:00");
		 	PaiShui();		//排水
			while(!shuiwei1)
			{
				w_cmd(0x80+7);	
        	    w_str(" PaiShui");
			}

		 PIN_PaiShui = 1; //关排水阀门		
	
}
/*****************甩干****************************/
void TuoShui()
{
	w_cmd(0x80+7);	
	w_str("TuoShui");
	PIN_JinShui = 1; //pai水控制 
    PIN_PaiShui = 0;
	flag_TuoShui=1;
    ET0=1;
	T_S =t3;
	fen=t3/60;miao=t3%60; 	     
	
	while(flag_TuoShui)
	{	 
		table7[0]=fen/10+0x30;		//显示剩余时间
		table7[1]=fen%10+0x30;
		table7[3]=miao/10+0x30;
	    table7[4]=miao%10+0x30;
		w_cmd(0xc0+7);	
		 w_str(table7);			 
	}
	//PIN_PaiShui = 1;
} 
/**********************报警程序************************/ 
void BaoJing(void) 
{  
	uint i, j;
	for (i = 0; i < 200; i++)
	{
         SPK = 0; for (j = 0; j < 100; j++);
         SPK = 1; for (j = 0; j < 100; j++);
    }	 
}
/**********************按键程序选择*******************/
void keyscan()
{
	
	if(K_SEL_ChengXu==0)		 //模式选择  
	{
		delay(10);		
		if(K_SEL_ChengXu==0)
		{
			while(!K_SEL_ChengXu);
		
			mol++;
			if(mol>1) mol=0;
		}
	}
	if(qiangruo==0)		 //模式选择  
	{
		delay(10);		
		if(qiangruo==0)
		{
			while(!qiangruo);
			flag_SEl_QiangRuo=~flag_SEl_QiangRuo;
			if(flag_SEl_QiangRuo) a=10;
			if(!flag_SEl_QiangRuo) a=20;			
		}
	} 	
}
/************************主程序************************/ 
void main() 
{ 
	
	uchar i; 
	ExInt_Init(); //外中断初始化 
	lcd_int();
	Timer0_Init();
	Timer1_Init(); 
//	ET0 = 1; 
	Enable(); //开总中断 
	while(1) 
	 {
		  
		
					 w_cmd(0x80+0x07);
	  			  	w_str("xyj");
				 
		 }
		   
	 
}
