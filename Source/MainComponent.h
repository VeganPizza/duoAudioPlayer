#pragma once

#include <JuceHeader.h>


//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
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


class MainComponent  : public juce::AudioAppComponent,
public juce::ChangeListener
{
public:
    //==============================================================================
    MainComponent()
        : state (Stopped)
    {
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
        
        decibelSlider.setRange (20, 20000);
        decibelSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 100, 20);
        decibelSlider.onValueChange = [this] {level = juce::Decibels::decibelsToGain ((float) decibelSlider.getValue()); };
        decibelSlider.setValue (juce::Decibels::gainToDecibels (level));
        decibelLabel.setText ("Volume in dB", juce::dontSendNotification);
        
        addAndMakeVisible (decibelSlider);
        addAndMakeVisible (decibelLabel);
        

        
        setSize(800, 500);
        
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
        auto* device = deviceManager.getCurrentAudioDevice();
        auto activeInputChannels = device->getActiveInputChannels();
        auto activeOutputChannels = device->getActiveOutputChannels();
        auto maxInputChannels = activeInputChannels.getHighestBit() + 1;
        auto maxOutputChannels = activeOutputChannels.getHighestBit() +1;
        
//        auto level = (float) levelSlider.getValue();
        
        auto currentLevel = level;
        auto levelScale = currentLevel * 2.0f;
        
        if (readerSource.get() == nullptr){
            bufferToFill.clearActiveBufferRegion();
            return;
        }
        transportSource.getNextAudioBlock (bufferToFill);
        
//        for (auto channel = 0; channel < bufferToFill.buffer->getNumChannels(); ++channel){
//            auto* buffer = bufferToFill.buffer->getWritePointer (channel, bufferToFill.startSample);
//
//            for (auto sample = 0; sample < bufferToFill.numSamples; ++sample) {
//                buffer[sample] = random.nextFloat() * levelScale - currentLevel;
//            }
//        }
        
        for (auto channel = 0; channel < maxOutputChannels; ++channel){
            if ((! activeOutputChannels[channel]) || maxInputChannels == 0){
                bufferToFill.buffer->clear (channel, bufferToFill.startSample, bufferToFill.numSamples);
            }
            else {
//                auto actualInputChannel = channel % maxInputChannels;

                if(! activeInputChannels[channel]){
                    bufferToFill.buffer->clear (channel, bufferToFill.startSample, bufferToFill.numSamples);
                }
                else{
                    //auto* inBuffer = bufferToFill.buffer->getReadPointer (actualInputChannel, bufferToFill.startSample);
                    auto* outBuffer = bufferToFill.buffer->getWritePointer (channel, bufferToFill.startSample);
                    for (auto sample = 0; sample < bufferToFill.numSamples; ++sample){
                        outBuffer[sample] = random.nextFloat() * levelScale - currentLevel;
                    }
                }
            }
        }
    }
    
    void releaseResources() override {
        transportSource.releaseResources();
    }


    void resized() override
    {
        titleLabel.setBounds (10, 10, getWidth()/2 - 10, 20);
        titleLabel_2.setBounds (getWidth()/2, 10, getWidth()/2 - 10, 20);
        openButton.setBounds (10, 40, getWidth()/2 - 10, 20);
        playButton.setBounds (10, 70, getWidth()/2 - 10, 20);
        stopButton.setBounds (10, 100, getWidth()/2 - 10, 20);
        decibelLabel.setBounds (getWidth()/2, 40, getWidth()/2 - 10, 20);
        decibelSlider.setBounds (getWidth()/2, 70, getWidth()/2 - 10, 20);

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
        chooser = std::make_unique<juce::FileChooser> ("Select sound file...", juce::File{}, "*.mp3");
        
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
    
    juce::Random random;
    DecibelSlider decibelSlider;
    juce::Label decibelLabel;
    float level = 0.0f;

    
    std::unique_ptr<juce::FileChooser> chooser;
    
    juce::AudioFormatManager formatManager;
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
    juce::AudioTransportSource transportSource;
    TransportState state;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
