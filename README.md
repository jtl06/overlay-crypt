# stm32g031 overlay demo
simple project to learn how overlays work.
Uses BearSSL library for RSA algorithm

Tested on STM32G031K8, running at 64Mhz. 

plan is to use overlay to fit code in memory that wouldn't otherwise fit due to 4KB sram on stm32g031k8
compare speed with overlay and compare speed running from flash.

WIP.
