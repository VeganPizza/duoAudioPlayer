/*
  ==============================================================================

    This file contains the basic startup code for a JUCE application.

  ==============================================================================
*/

#include <JuceHeader.h>
#include "MainComponent.h"

//==============================================================================
class duoAudioPlayerApplication  : public juce::JUCEApplication
{
public:
    //==============================================================================
    duoAudioPlayerApplication() = default;

    const juce::String getApplicationName() override       { return "duoAudioPlayer"; }
    const juce::String getApplicationVersion() override    { return "1.0.0"; }

    //==============================================================================
    void initialise (const juce::String&) override
    {
        mainWindow.reset (new MainWindow ("duoAudioPlayer", new MainComponent, *this));
    }

    void shutdown() override
    {
        mainWindow = nullptr; // (deletes our window)
    }
private:
    class MainWindow    : public juce::DocumentWindow
    {
    public:
        MainWindow (const juce::String& name, juce::Component* c, JUCEApplication& a)
            : DocumentWindow (name, juce::Desktop::getInstance().getDefaultLookAndFeel()
                                                                .findColour (ResizableWindow::backgroundColourId),
                              juce::DocumentWindow::allButtons),
              app (a)
        {
            setUsingNativeTitleBar (true);
            setContentOwned (c, true);

           #if JUCE_IOS || JUCE_ANDROID
            setFullScreen (true);
           #else
            setResizable (true, true);
            //setResizeLimits (300, 250, 10000, 10000);
            centreWithSize (getWidth(), getHeight());
           #endif

            setVisible (true);
        }

        void closeButtonPressed() override
        {
            app.systemRequestedQuit();
        }

    private:
        JUCEApplication& app;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainWindow)
    };

private:
    std::unique_ptr<MainWindow> mainWindow;
};

//==============================================================================
// This macro generates the main() routine that launches the app.
START_JUCE_APPLICATION (duoAudioPlayerApplication)
