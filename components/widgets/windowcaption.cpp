#include "windowcaption.hpp"

#include <stdexcept>

namespace Gui
{

    WindowCaption::WindowCaption()
        : mLeft(nullptr)
        , mRight(nullptr)
        , mClient(nullptr)
    {
    }

    void WindowCaption::initialiseOverride()
    {
        Base::initialiseOverride();

        assignWidget(mLeft, "Left");
        assignWidget(mRight, "Right");

#if MYGUI_VERSION < MYGUI_DEFINE_VERSION(3,2,3) // name changed in commit b88ca1b
        assignWidget(mClient, "Client");
        if (!mClient)
            throw std::runtime_error("WindowCaption needs an EditBox Client widget in its skin");
#else
		assignWidget(mScrollViewClient, "Client");
		if (!mScrollViewClient)
			throw std::runtime_error("WindowCaption needs an EditBox Client widget in its skin");
#endif
    }

    void WindowCaption::setCaption(const MyGUI::UString &_value)
    {
        EditBox::setCaption(_value);
        align();
    }

    void WindowCaption::setSize(const MyGUI::IntSize& _value)
    {
        Base::setSize(_value);
        align();
    }

    void WindowCaption::setCoord(const MyGUI::IntCoord& _value)
    {
        Base::setCoord(_value);
        align();
    }

    void WindowCaption::align()
    {
        MyGUI::IntSize textSize = getTextSize();
#if MYGUI_VERSION < MYGUI_DEFINE_VERSION(3, 2, 3) // name changed in commit b88ca1b
        MyGUI::Widget* caption = mClient;
#else
		MyGUI::Widget* caption = mScrollViewClient;
#endif
        caption->setSize(textSize.width + 24, caption->getHeight());

        int barwidth = (getWidth()-caption->getWidth())/2;
        caption->setPosition(barwidth, caption->getTop());
        if (mLeft)
            mLeft->setCoord(0, mLeft->getTop(), barwidth, mLeft->getHeight());
        if (mRight)
            mRight->setCoord(barwidth + caption->getWidth(), mRight->getTop(), barwidth, mRight->getHeight());
    }

}
