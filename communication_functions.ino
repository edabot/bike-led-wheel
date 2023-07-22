void updateWheelFromChars ()
{
  int mode = incomingChars[1] - '0';
  int subMode = (incomingChars[2] - '0') % 30;
  boolean reversed = incomingChars[2] - '0' >= 30;
  updateWheel(mode, subMode, reversed);

} // end of processIncomingByte

void ProcessIncomingChar (const uint8_t c) {
    if (c - '0' >= 0) { 
    incomingChars[incomingIdx] = c;
    if (c == 'X'){
      for (int i = 0; i <= incomingIdx; i++) {
        Serial.print(incomingChars[i]);
      }
      if (incomingIdx < MESSAGE_LENGTH) {
        Serial1.print("R");
        Serial.println("R");
      } else {
        Serial.print("\n");
        updateWheelFromChars();
      }
      incomingIdx = 0;
    } else {
      incomingIdx++;        
    }
    }
}

void handlePreviousState ()
{
  switch (state)
  {
  case GOT_M:
    nextDisplayMode = currentValue;
    break;
  case GOT_S:
    nextSubMode = currentValue;
    break;
  case GOT_X:
    updateWheel(nextDisplayMode, nextSubMode, true);
    break;
  }  // end of switch  

  currentValue = 0; 
} 
