#include "stm32f4xx.h"                  // Device header
#include "stm32f4xx_dcmi.h"
#include "dcmi.h" 
#include "ov7670.h" 
#include "stdio.h"
#include "string.h"
//#include "sys_cfg.h"
//////////////////////////////////////////////////////////////////////////////////	 
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//ALIENTEK STM32F407开发板
//DCMI 驱动代码	   
//正点原子@ALIENTEK
//技术论坛:www.openedv.com
//创建日期:2014/5/14
//版本：V1.0
//版权所有，盗版必究。
//Copyright(C) 广州市星翼电子科技有限公司 2014-2024
//All rights reserved									  
////////////////////////////////////////////////////////////////////////////////// 	 

u8 ov_frame=0;  						//帧率
u32 datanum=0;
u32 HSYNC=0;
u32 VSYNC=0;
DCMI_InitTypeDef DCMI_InitStructure;
u8 ov_rev_ok = 0;

//DCMI DMA配置
//DMA_Memory0BaseAddr:存储器地址    将要存储摄像头数据的内存地址(也可以是外设地址)
//DMA_BufferSize:存储器长度    0~65535
//DMA_MemoryDataSize:存储器位宽  
//DMA_MemoryDataSize:存储器位宽    @defgroup DMA_memory_data_size :DMA_MemoryDataSize_Byte/DMA_MemoryDataSize_HalfWord/DMA_MemoryDataSize_Word
//DMA_MemoryInc:存储器增长方式  @defgroup DMA_memory_incremented_mode  /** @defgroup DMA_memory_incremented_mode : DMA_MemoryInc_Enable/DMA_MemoryInc_Disable
void DCMI_DMA_Init(u32 DMA_Memory0BaseAddr,u16 DMA_BufferSize,u32 DMA_MemoryDataSize,u32 DMA_MemoryInc)
{ 
	DMA_InitTypeDef  DMA_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
  	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2,ENABLE);//DMA2时钟使能 
	DMA_DeInit(DMA2_Stream1);
	while (DMA_GetCmdStatus(DMA2_Stream1) != DISABLE){}//等待DMA2_Stream1可配置 
	
  	/* 配置 DMA Stream */
  	DMA_InitStructure.DMA_Channel = DMA_Channel_1;  //通道1 DCMI通道 
  	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&DCMI->DR; ;//外设地址为:DCMI->DR
  	DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)DMA_Memory0BaseAddr;//DMA 存储器0地址
  	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;//外设到存储器模式
  	DMA_InitStructure.DMA_BufferSize = DMA_BufferSize;//数据传输量 
  	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;//外设非增量模式
  	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc;//存储器增量模式
  	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Word;//外设数据长度:32位
  	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize;//存储器数据长度 
  	DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;// 使用循环模式  DMA_Mode_Normal
  	DMA_InitStructure.DMA_Priority = DMA_Priority_High;//高优先级
  	DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Enable; //FIFO模式        
  	DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_Full;//使用全FIFO 
  	DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;//外设突发单次传输
  	DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;//存储器突发单次传输
  	DMA_Init(DMA2_Stream1, &DMA_InitStructure);//初始化DMA Stream
		
	DMA_ITConfig(DMA2_Stream1,DMA_IT_TC,ENABLE);
	NVIC_InitStructure.NVIC_IRQChannel=	DMA2_Stream1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority=0;
	NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE;
	NVIC_Init(&NVIC_InitStructure);	//根据指定的参数初始化VIC寄存器、
	
} 

void DMA2_Stream1_IRQHandler(void)
{        
	if(DMA_GetFlagStatus(DMA2_Stream1,DMA_FLAG_TCIF1)==SET)//DMA2_Steam1,传输完成标志
	{  
		DMA_Cmd(DMA2_Stream1, DISABLE);	
		
		DMA_ClearFlag(DMA2_Stream1,DMA_FLAG_TCIF1);//清除传输完成中断
		
		ov_rev_ok= 1;
		
		datanum++;
	}    											 
} 

//DCMI初始化
void My_DCMI_Init(void)
{
  	GPIO_InitTypeDef GPIO_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
  	RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOA |RCC_AHB1Periph_GPIOB|RCC_AHB1Periph_GPIOC|RCC_AHB1Periph_GPIOD | RCC_AHB1Periph_GPIOH, ENABLE);
	
	RCC_AHB2PeriphClockCmd(RCC_AHB2Periph_DCMI,ENABLE);//使能DCMI时钟
	/*
	HREF PH8
	DCMI_PCLK  PA6
	DCMI_VSYNC  PB7
	DCMI_D[7:0] 0]，即 PB9/PB8 /PD3 /PC11/PC9/PC8/PC7/PC6
	
	**/
	
	
  	// HREF PH8
  	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;//   复用功能输出 HSYNC 
  	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF; //复用功能输出
  	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//推挽输出
  	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
  	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//上拉
  	GPIO_Init(GPIOH, &GPIO_InitStructure);//初始化
	
	// DCMI_PCLK  PA6
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;///6   复用功能输出  PIXCLK 
  	GPIO_Init(GPIOA, &GPIO_InitStructure);//初始化
	
  	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;// PB7   复用功能输出 VSYNC D5 
  	GPIO_Init(GPIOB, &GPIO_InitStructure);//初始化
	
  	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6|GPIO_Pin_7|GPIO_Pin_8|GPIO_Pin_9 | GPIO_Pin_11;//PC6/7/8/9/11 复用功能输出 D0 D1 D2 D3 D4
  	GPIO_Init(GPIOC, &GPIO_InitStructure);//初始化

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;//PD3  复用功能输出  D3
  	GPIO_Init(GPIOD, &GPIO_InitStructure);//初始化

  	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8|GPIO_Pin_9;//PE8/9  复用功能输出  D8 D9
  	GPIO_Init(GPIOE, &GPIO_InitStructure);//初始化	

	GPIO_PinAFConfig(GPIOH,GPIO_PinSource8,GPIO_AF_DCMI); //PH8,AF13  DCMI_HSYNC
	GPIO_PinAFConfig(GPIOA,GPIO_PinSource6,GPIO_AF_DCMI); //PA6,AF13  DCMI_PCLK  
 	GPIO_PinAFConfig(GPIOB,GPIO_PinSource7,GPIO_AF_DCMI); //PB7,AF13  DCMI_VSYNC 
 	GPIO_PinAFConfig(GPIOC,GPIO_PinSource6,GPIO_AF_DCMI); //PC6,AF13  DCMI_D0  
 	GPIO_PinAFConfig(GPIOC,GPIO_PinSource7,GPIO_AF_DCMI); //PC7,AF13  DCMI_D1 
	GPIO_PinAFConfig(GPIOC,GPIO_PinSource8,GPIO_AF_DCMI); //PC8,AF13  DCMI_D2
	GPIO_PinAFConfig(GPIOC,GPIO_PinSource9,GPIO_AF_DCMI); //PC9,AF13  DCMI_D3
	GPIO_PinAFConfig(GPIOC,GPIO_PinSource11,GPIO_AF_DCMI); //PC11,AF13  DCMI_D4 
	GPIO_PinAFConfig(GPIOD,GPIO_PinSource3,GPIO_AF_DCMI); //PD3,AF13  DCMI_D5 
	GPIO_PinAFConfig(GPIOB,GPIO_PinSource8,GPIO_AF_DCMI); //PB8,AF13  DCMI_D6
	GPIO_PinAFConfig(GPIOB,GPIO_PinSource9,GPIO_AF_DCMI); //PB9,AF13  DCMI_D7

	DCMI_DeInit();//清除原来的设置 
 
  	DCMI_InitStructure.DCMI_CaptureMode=DCMI_CaptureMode_SnapShot;//快照模式 DCMI_CaptureMode_SnapShot
	DCMI_InitStructure.DCMI_CaptureRate=DCMI_CaptureRate_All_Frame;//全帧捕获
	DCMI_InitStructure.DCMI_ExtendedDataMode= DCMI_ExtendedDataMode_8b;//8位数据格式  
	DCMI_InitStructure.DCMI_HSPolarity = DCMI_HSPolarity_Low;//DCMI_HSPolarity_High;//HSYNC 低电平有效
	DCMI_InitStructure.DCMI_PCKPolarity= DCMI_PCKPolarity_Falling;//PCLK 下升沿有效
	DCMI_InitStructure.DCMI_SynchroMode= DCMI_SynchroMode_Hardware;//硬件同步HSYNC,VSYNC
	DCMI_InitStructure.DCMI_VSPolarity=DCMI_VSPolarity_High;//VSYNC 低电平有效
	DCMI_Init(&DCMI_InitStructure);

	DCMI_ITConfig(DCMI_IT_FRAME,ENABLE);//开启帧中断 
	DCMI_ITConfig(DCMI_IT_LINE,ENABLE); //开启行中断
	DCMI_ClearITPendingBit(DCMI_IT_VSYNC);
	DCMI_ITConfig(DCMI_IT_VSYNC,ENABLE); //开启场中断	
	DCMI_Cmd(ENABLE);	//DCMI使能

  	NVIC_InitStructure.NVIC_IRQChannel = DCMI_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=3;	//抢占优先级
	NVIC_InitStructure.NVIC_IRQChannelSubPriority =2;		//子优先级
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);	//根据指定的参数初始化NVIC寄存器、
} 

//DCMI,启动传输
void DCMI_Start(void)
{ 
	//开始写入GRAM
	DMA_Cmd(DMA2_Stream1, ENABLE);//开启DMA2,Stream1 
	DCMI_CaptureCmd(ENABLE);//DCMI捕获使能  
}

//DCMI,关闭传输
void DCMI_Stop(void)
{ 
  	DCMI_CaptureCmd(DISABLE);//DCMI捕获使关闭	
	while(DCMI->CR&0X01);		//等待传输结束  	
	DMA_Cmd(DMA2_Stream1,DISABLE);//关闭DMA2,Stream1
} 

//DCMI中断服务函数
void DCMI_IRQHandler(void)
{

	if(DCMI_GetITStatus(DCMI_IT_LINE)==SET)//捕获到行
	{
		DCMI_ClearITPendingBit(DCMI_IT_LINE);//清除中断	
		ov_frame++;
	}
	
	if(DCMI_GetITStatus(DCMI_IT_FRAME)==SET)//捕获到帧
	{	
		DCMI_ClearITPendingBit(DCMI_IT_FRAME);//清除中断	
	}
	
	if(DCMI_GetITStatus(DCMI_IT_VSYNC)==SET)//捕获到场
	{	
		DCMI_ClearITPendingBit(DCMI_IT_VSYNC);//清除中断		
	}
} 

////////////////////////////////////////////////////////////////////////////////
//以下两个函数,供usmart调用,用于调试代码

//DCMI设置显示窗口
//sx,sy;LCD的起始坐标
//width,height:LCD显示范围.
void DCMI_Set_Window(u16 sx,u16 sy,u16 width,u16 height)
{
	DCMI_Stop(); 
	
	DMA_Cmd(DMA2_Stream1,ENABLE);	//开启DMA2,Stream1 
	
	DCMI_CaptureCmd(ENABLE);//DCMI捕获使能 
	
}


