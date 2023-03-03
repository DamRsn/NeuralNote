#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <thread>

using namespace juce;

namespace GuiApp
{
//Helpers to calculate the wavetable path
namespace PathCalcs
{
    inline float getY(float x, float freq)
    {
        return std::sin(x * MathConstants<float>::twoPi * freq);
    }

    inline Point<float> getPoint(int x, float freq, Rectangle<float> bounds)
    {
        auto scaledX = jmap((float) x, 0.f, bounds.getWidth(), 0.f, 1.f);
        auto y = getY(scaledX, freq);
        auto scaledY = jmap(y, -1.f, 1.f, 0.f, bounds.getHeight());

        return {(float) x, scaledY};
    }

    inline Path getPath(Rectangle<int> bounds, float freq)
    {
        Path wavePath;
        wavePath.startNewSubPath(getPoint(0, freq, bounds.toFloat()));

        for (int x = 1; x < bounds.getWidth(); ++x)
            wavePath.lineTo(getPoint(x, freq, bounds.toFloat()));

        return wavePath;
    }

    inline void paintPath(Graphics& g, Rectangle<int> bounds, float freq)
    {
        auto path = getPath(bounds, freq);
        g.setColour(Colours::lightblue);
        g.strokePath(path, PathStrokeType(1.0f));
    }
} // namespace PathCalcs

//Checks for the existance of "Desktop/Threading.txt" as a flag for the threading
inline bool shouldUseThreading()
{
    auto config = File::getSpecialLocation(File::userDesktopDirectory)
                      .getChildFile("Threading.txt");

    return config.existsAsFile();
}

//A simple "Job", storing the frequency and scale from whoever dispatched the job
struct PaintJobInfo
{
    PaintJobInfo() = default;
    PaintJobInfo(float freqToUse, float scaleToUse)
        : freq(freqToUse)
        , scale(scaleToUse)
    {
    }
    bool run(Image& result, Rectangle<float> bounds) const noexcept
    {
        if (scale > 0.f)
        {
            auto scaledBounds = bounds * scale;
            auto intBounds = scaledBounds.toNearestInt();

            //We have to scale the image here when we know the real scale factor:
            if (result.getBounds() != intBounds)
            {
                result = Image(Image::PixelFormat::ARGB,
                               intBounds.getWidth(),
                               intBounds.getHeight(),
                               true);
            }
            else
            {
                result.clear(intBounds);
            }

            Graphics g(result);

            g.addTransform(
                AffineTransform::scale(scaledBounds.getWidth() / bounds.getWidth(),
                                       scaledBounds.getHeight() / bounds.getHeight()));

            PathCalcs::paintPath(g, bounds.toNearestInt(), freq);

            return true;
        }

        return false;
    }

    float freq = 0.f;
    float scale = 0.f;
};

//A painter thread class.
//This thread runs at some frequency on another thread, for example 100hz
//
//And paints the last 'job' sent to it.
//The thread will only look at the very latest job so it's finen to pass jobs to it
//At a higher or lower rate
struct PaintThread
{
    PaintThread(Component& parentToUse)
        : parent(parentToUse)
    {
        thread = std::make_unique<std::thread>(
            [&]
            {
                while (running.load())
                {
                    hiResTimerCallback();
                    Thread::sleep(10);
                }
            });
    }

    ~PaintThread()
    {
        running.store(false);
        thread->join();
    }

    void hiResTimerCallback()
    {
        PaintJobInfo jobToDo;

        {
            ScopedLock sl(lock);
            jobToDo = nextJob;
            nextJob = {};
        }

        //Still on the side thread, runs the pain job:
        if (jobToDo.run(threadImage, bounds))
        {
            //When the job is finished, we send an async call (message thread)
            //To blend the image back into the dispatched component
            auto imageCopy = threadImage.createCopy();

            MessageManager::callAsync(
                [imageCopy, this]
                {
                    cachedImage = imageCopy;
                    parent.repaint();
                });
        }
    }

    //Passes the job into the line by copy
    void addJob(const PaintJobInfo& jobToUse)
    {
        ScopedLock sl(lock);
        nextJob = jobToUse;
    }

    void setBounds(Rectangle<int> boundsToUse) { bounds = boundsToUse.toFloat(); }

    Component& parent;
    Image threadImage;
    Image cachedImage;
    Rectangle<float> bounds;
    CriticalSection lock;
    PaintJobInfo nextJob;
    std::unique_ptr<std::thread> thread;
    std::atomic<bool> running {true};
};

struct ComplicatedPath
    : public Component
    , public Timer
{
    ComplicatedPath() { startTimerHz(100); }

    void timerCallback() override
    {
        //Randomly changes frequency...
        static Random rand;
        auto offset = jmap(rand.nextFloat(), 0.f, 0.1f);

        frequency += offset;
        frequency = fmod(frequency, 10.f);

        //If we're multithreading, we're dispatching all data by copy into the thread:
        if (shouldUseThreading())
            thread.addJob({frequency, scaleFactor});
        else
            repaint();
    }

    void resized() override { thread.setBounds(getLocalBounds()); }

    void paint(Graphics& g) override
    {
        //We need to store the "real" scale factor so we can use it in out paint later...
        scaleFactor = g.getInternalContext().getPhysicalPixelScaleFactor();

        //If we're multithreading, we're just painting a precalculated image:
        if (shouldUseThreading())
            g.drawImage(thread.cachedImage, getLocalBounds().toFloat());
        else
            PathCalcs::paintPath(g, getLocalBounds(), frequency);
    }

    PaintThread thread {*this};
    float scaleFactor = 1.f;
    float frequency = 0.f;
};

class MainComponent : public Component
{
public:
    MainComponent();

    void paint(Graphics&) override;
    void resized() override;

private:
    std::vector<std::unique_ptr<ComplicatedPath>> paths;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};

} // namespace GuiApp
