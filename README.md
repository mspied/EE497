# EE497
Senior Design Project Semester 1

The Demo folder contains the local mqtt client script used for the in person demo, along with the data we recorded with the ADAS1000 for use in the demo. MQTT connection info was removed.

The ECG-FM folder contains the repository for the AI we are making use of at https://github.com/bowang-lab/ECG-FM/tree/main
It also contains our test scripts and the demo client that connected with MQTT to the local client to recieve files, run the inference, and return the results.
The checkpoint for the model is not included in the folder as it is too large to upload but can be found on the original github

The ECG-FM/ourdata folder also contains some data recorded with our ADAS1000, modified for testing with the model.
