#pragma once

//#include <JuceHeader.h>

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
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
        
        setSize(800, 500);
        
        formatManager.registerBasicFormats();
        transportSource.addChangeListener (this);   
        
        setAudioChannels(0, 2);
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
        if (readerSource.get() == nullptr){
            bufferToFill.clearActiveBufferRegion();
            return;
        }
        transportSource.getNextAudioBlock (bufferToFill);
    }
    
    void releaseResources() override {
        transportSource.releaseResources();
    }


    void resized() override
    {
        titleLabel.setBounds (10, 10, getWidth()/2 - 10, 20);
        openButton.setBounds (10, 40, getWidth()/2 - 10, 20);
        playButton.setBounds (10, 70, getWidth()/2 - 10, 20);
        stopButton.setBounds (10, 100, getWidth()/2 - 10, 20);
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
    
    juce::TextButton openButton;
    juce::TextButton playButton;
    juce::TextButton stopButton;
    
    std::unique_ptr<juce::FileChooser> chooser;
    
    juce::AudioFormatManager formatManager;
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
    juce::AudioTransportSource transportSource;
    TransportState state;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
