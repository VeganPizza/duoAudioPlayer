#pragma once

#include <JuceHeader.h>


//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.

class DecibelSlider : public juce::Slider{
public:
    DecibelSlider() {}
    
    double getValueFromText (const juce::String& text) override{
        
        auto minusInfinitydB = -100.0;
        
        auto decibelText = text.upToFirstOccurrenceOf ("dB", false, false).trim();
        
        return decibelText.equalsIgnoreCase ("-INF") ? minusInfinitydB
        : decibelText.getDoubleValue();
    }
    
    juce::String getTextFromValue (double value) override {
        return juce::Decibels::toString (value);
    }
    
private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DecibelSlider)
    
};

 */
class MainComponent  : public juce::AudioAppComponent,
public juce::ChangeListener
{
public:
    //==============================================================================
    MainComponent()
        : juce::AudioAppComponent(deviceManager), state (Stopped)
    {
        
        //labels
        addAndMakeVisible(titleLabel);
        titleLabel.setFont (juce::Font (16.0f, juce::Font::bold));
        titleLabel.setText ("Play sound from existing file", juce::dontSendNotification);
        titleLabel.setColour (juce::Label::textColourId, juce::Colours::white);
        titleLabel.setJustificationType (juce::Justification::centred);
        
        addAndMakeVisible(titleLabel_2);
        titleLabel_2.setFont (juce::Font (16.0f, juce::Font::bold));
        titleLabel_2.setText ("Sound playing from device", juce::dontSendNotification);
        titleLabel_2.setColour (juce::Label::textColourId, juce::Colours::white);
        titleLabel_2.setJustificationType (juce::Justification::centred);
        
        //buttons & sliders for existing file
        addAndMakeVisible (&openButton);
        openButton.setButtonText ("Open...");
        openButton.onClick = [this] { openButtonClicked(); };

        addAndMakeVisible (&playButton);
        playButton.setButtonText ("Play");
        playButton.onClick = [this] { playButtonClicked(); };
        playButton.setColour (juce::TextButton::buttonColourId, juce::Colours::green);
        playButton.setEnabled (false);

        addAndMakeVisible (&stopButton);
        stopButton.setButtonText ("Stop");
        stopButton.onClick = [this] { stopButtonClicked(); };
        stopButton.setColour (juce::TextButton::buttonColourId, juce::Colours::red);
        stopButton.setEnabled (false);
        
        //buttons & sliders for input channels

        deviceManager.initialise(2, 2, nullptr, true);
        audioSettings.reset(new juce::AudioDeviceSelectorComponent(deviceManager, 0, 2, 0, 2, true, true, true, true));
        addAndMakeVisible(audioSettings.get());


        
        setSize(1000, 500);
        
        formatManager.registerBasicFormats();
        transportSource.addChangeListener (this);   
        
        setAudioChannels(2, 2);
    }

    ~MainComponent() override
    {
        // This shuts down the audio device and clears the audio source.
        shutdownAudio();
    }

    //==============================================================================
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override
    {
        transportSource.prepareToPlay(samplesPerBlockExpected, sampleRate);
    }

    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override
    {
       
 
        
        bufferToFill.clearActiveBufferRegion();
        transportSource.getNextAudioBlock (bufferToFill);

    }
    
    void releaseResources() override {
        transportSource.releaseResources();
    }


    void resized() override
    {
        titleLabel.setBounds (10, 10, getWidth()/2 - 10, 20);
        titleLabel_2.setBounds (getWidth()/2, 10, getWidth()/2 - 10, 20);
        openButton.setBounds (10, getHeight() - 90, getWidth()/2 - 10, 20);
        playButton.setBounds (10, getHeight() - 60, getWidth()/2 - 10, 20);
        stopButton.setBounds (10, getHeight() - 30, getWidth()/2 - 10, 20);
        audioSettings->setBounds (getWidth()/2, getHeight()/2 - 100, getWidth()/2 - 10, 100);

    }
    
    void changeListenerCallback (juce::ChangeBroadcaster* source) override {
        if (source == &transportSource){
            if (transportSource.isPlaying())
                changeState (Playing);
            else
                changeState (Stopped);
        }
    }

private:
    
    enum TransportState{
        Stopped,
        Starting,
        Playing,
        Stopping
    };
    
    void changeState (TransportState newState){
        if (state != newState){
            state = newState;
            
            switch (state){
                case Stopped:
                    stopButton.setEnabled (false);
                    playButton.setEnabled (true);
                    transportSource.setPosition (0.0);
                    break;
                    
                case Starting:
                    playButton.setEnabled (false);
                    transportSource.start();
                    break;
                    
                case Playing:
                    stopButton.setEnabled (true);
                    break;
                    
                case Stopping:
                    transportSource.stop();
                    break;
            }
        }
    }
    
    void openButtonClicked(){
        chooser = std::make_unique<juce::FileChooser> ("Select sound file...", juce::File{}, "*.mp3; *.wav");
        
        auto chooserFlags = juce::FileBrowserComponent::openMode
        | juce::FileBrowserComponent::canSelectFiles;
        
        chooser->launchAsync(chooserFlags, [this] (const juce::FileChooser& fc){
            auto file = fc.getResult();
            
            if (file != juce::File{}){
                auto* reader = formatManager.createReaderFor (file);
                
                if (reader != nullptr){
                    auto newSource = std::make_unique<juce::AudioFormatReaderSource> (reader, true);
                    transportSource.setSource (newSource.get(), 0, nullptr, reader->sampleRate);
                    playButton.setEnabled (true);
                    readerSource.reset (newSource.release());
                }
            }
        });
    }
    
    void playButtonClicked() {changeState (Starting);};
    
    void stopButtonClicked() {changeState (Stopping);};
    
    juce::Label titleLabel;
    juce::Label titleLabel_2;
    
    juce::TextButton openButton;
    juce::TextButton playButton;
    juce::TextButton stopButton;
    

    
    std::unique_ptr<juce::FileChooser> chooser;
    
    juce::AudioDeviceManager deviceManager;
    std::unique_ptr<juce::AudioDeviceSelectorComponent> audioSettings;

    juce::AudioFormatManager formatManager;
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
    juce::AudioTransportSource transportSource;
    TransportState state;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
