# hypnos

Module embedded inside users' laptop to "remind" them to take break and sleep

## Background

I used to have a script that runs every couple hours to turn the computer off to remind me to take a break. However, that script can be easily bypassed and it's not cross-platform. Embedding ```hypnos``` into the motherboard of my laptop and enabling tamperproof protection in the BIOS for the case make it much harder to bypass.

## Features

- Interact with device through phone app
    - Allow a limited number of exceptions per month
    - Customizable work and break interval(there has to be an upper limit though)
    - Statistics for tracking progress
    - Foreground task to show device status
    - Battery indicator
- Flexibly adapt to user' start of day
- Hard to abuse or bypass but also possible to remove when truly necessary
- Low power consumption
- Small size
