App for a Digital command zone.



The Hardware consists of a rotary encoder to cycle through the different values.



When the rotary encoder is spun the index of the values is changed to display the selected value.



After the rotary encoder comes to rest for a set period of time the button listener state becomes active.



In button listener state buttons are used to increment or decrement the value of the current index.



This state waits increments and decrements a counter when buttons are pressed then adds that counter to the value on timeout.



