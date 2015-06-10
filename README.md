
                          Asteroids
                          --------


0) Intro
This is a merge of asteroids and naudio. The result is a playable game
on the STM32F4discovery board 

1) Setup
- Flash main.axf to the board
- Wire up a 4bit ladder dac from GPIOC[3:0] to the ladder dac (curr
  its either max or 0 in the code) 
- controls are on GPIOB[5:0]={left,right,thust,fire,hyperspace,selftest}
- GPIOC[5:4] are for the start LEDs
- GPIOB[8:7]={start2,start1}
- GPIOC[7] = GPIO[6]. Please ensure a jumper block is placed to short
  those two pins together. This ties MCLK to an internal timer for
  audio to function correctly.
- Make sure that HSE_VALUE in:
  Libraries/CMSIS/ST/STM32F4xx/Include/stm32f4xx.h
  is 8Mhz for the discovery board. The library as shipped has 25Mhz
  there. 
2) On performance
  Vector workload
  Typical workload is ~3k points/(~200 on and 100 off vectors) every
  60hz. This can peak to ~12000 points/(~500 on and ~300 off) during
  highscore initials prompting.
  Audio workload
  At any given time when the game is up, audio is processing. Using a
  divide down from 12.288Mhz by 256 gives 48khz exactly. That clock
  drives an interrupt handler which processes 1 sample every
  clock. Audio is double buffer, circular DMA. A very small buffer is
  in play for that. Samples are mixed in mixer.s and can come from up
  to 10 sources. Typically the mix is not that broad but it is capable
  of that that wider mix if needed.

