# Hopscotch-LED-matrix-
Controlling 2002 LEDs in a 14x11x13 matrix 

http://letshopscotch.com
Installation runs until March 31st 2019 in Austin. 

Finished installation https://www.instagram.com/p/Bt9JXZABl3h/

Hopscotch, a new experiential art installation group in Austin, approached me with a loose plan to to construct a matrix of the strands of LED balls you can find on ebay or alibaba. 

The main hurdle with this type of setup is that the led strands are not continuous. Some manufacturers offer strands with a return data line so the end can be capped and the data line can run back up the strip and sent to the next but it's not at all shielded and every strand of these I tried had glitching problems. 

The solution was to split up the incoming artnet data and send it to each strip separately. I could have gone with off the shelf solutions for this but it would be very costly for 143 stands. So instead I made a simple custom controller for each row. 

<img src="https://raw.githubusercontent.com/BleepLabs/Hopscotch-LED-matrix-/master/hopscotch-digram.jpg">

Each controller consisted of a wiz850io, Teensy 3.2, and 74HC244 to shift the 3v3 levels to 5. I could have probably used a 3.5 and controlled two rows with a single unit but time was short. This worked well in testing, was felxible, and was still much cheaper than the alternatives. Unfortunatley due to time constraints and the event setup happening during lunar new year I had to make them by hand on protoboard. 

Each controller has it's own ip and would send one universe of data to one side of LEDs (273 elements) and another to the other side as together it would be over the 512 DMX channels max. 

All the controllers were connected with cat5 to two simple ethernet switches  https://www.tp-link.com/us/products/details/cat-5582_TL-SF1008D.html. Both switches were connected together and a single line went to a PC running TouchDesigner. In touch it was a simple but tedious jot of setting up all the strands in 3d and assigning them the proper ips and universe numbers. This and the visual art was handled by www.doda.me and is a whole other story. TouchDesigner also played back a quadraphoic audio piece composed by Malika Boudisa. 

Each row had a seperate power supply for the left and right side. 
https://www.mouser.com/ProductDetail/709-LPV60-12
Each row could consume over 9A at 12V DC when fully white. This configuration only get us 10% headroom but these supplies are very reliable and can be run at their max.

The entire setup could pull over 30A from mains so two independent 16A circuits were used. Pretty close to max but after texting no issuses came up even at full white during turn on 



