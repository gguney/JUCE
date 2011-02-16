/*
  ==============================================================================

   This file is part of the JUCE library - "Jules' Utility Class Extensions"
   Copyright 2004-10 by Raw Material Software Ltd.

  ------------------------------------------------------------------------------

   JUCE can be redistributed and/or modified under the terms of the GNU General
   Public License (Version 2), as published by the Free Software Foundation.
   A copy of the license is included in the JUCE distribution, or can be found
   online at www.gnu.org/licenses.

   JUCE is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
   A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

  ------------------------------------------------------------------------------

   To release a closed-source product which uses JUCE, commercial licenses are
   available: visit www.rawmaterialsoftware.com/juce for more information.

  ==============================================================================
*/

#include "../../../core/juce_StandardHeader.h"

BEGIN_JUCE_NAMESPACE

#include "juce_RelativeRectangle.h"
#include "juce_RelativeCoordinatePositioner.h"


//==============================================================================
namespace RelativeRectangleHelpers
{
    inline void skipComma (String::CharPointerType& s)
    {
        s = s.findEndOfWhitespace();

        if (*s == ',')
            ++s;
    }

    bool dependsOnSymbolsOtherThanThis (const Expression& e)
    {
        if (e.getType() == Expression::operatorType && e.getSymbolOrFunction() == ".")
            return true;

        if (e.getType() == Expression::symbolType)
        {
            switch (RelativeCoordinate::StandardStrings::getTypeOf (e.getSymbolOrFunction()))
            {
                case RelativeCoordinate::StandardStrings::x:
                case RelativeCoordinate::StandardStrings::y:
                case RelativeCoordinate::StandardStrings::left:
                case RelativeCoordinate::StandardStrings::right:
                case RelativeCoordinate::StandardStrings::top:
                case RelativeCoordinate::StandardStrings::bottom:   return false;
                default: break;
            }

            return true;
        }
        else
        {
            for (int i = e.getNumInputs(); --i >= 0;)
                if (dependsOnSymbolsOtherThanThis (e.getInput(i)))
                    return true;
        }

        return false;
    }
}

//==============================================================================
RelativeRectangle::RelativeRectangle()
{
}

RelativeRectangle::RelativeRectangle (const RelativeCoordinate& left_, const RelativeCoordinate& right_,
                                      const RelativeCoordinate& top_, const RelativeCoordinate& bottom_)
    : left (left_), right (right_), top (top_), bottom (bottom_)
{
}

RelativeRectangle::RelativeRectangle (const Rectangle<float>& rect)
    : left (rect.getX()),
      right (Expression::symbol (RelativeCoordinate::Strings::left) + Expression ((double) rect.getWidth())),
      top (rect.getY()),
      bottom (Expression::symbol (RelativeCoordinate::Strings::top) + Expression ((double) rect.getHeight()))
{
}

RelativeRectangle::RelativeRectangle (const String& s)
{
    String::CharPointerType text (s.getCharPointer());
    left = RelativeCoordinate (Expression::parse (text));
    RelativeRectangleHelpers::skipComma (text);
    top = RelativeCoordinate (Expression::parse (text));
    RelativeRectangleHelpers::skipComma (text);
    right = RelativeCoordinate (Expression::parse (text));
    RelativeRectangleHelpers::skipComma (text);
    bottom = RelativeCoordinate (Expression::parse (text));
}

bool RelativeRectangle::operator== (const RelativeRectangle& other) const throw()
{
    return left == other.left && top == other.top && right == other.right && bottom == other.bottom;
}

bool RelativeRectangle::operator!= (const RelativeRectangle& other) const throw()
{
    return ! operator== (other);
}

//==============================================================================
// An expression context that can evaluate expressions using "this"
class RelativeRectangleLocalScope  : public Expression::Scope
{
public:
    RelativeRectangleLocalScope (const RelativeRectangle& rect_)  : rect (rect_) {}

    const Expression getSymbolValue (const String& symbol) const
    {
        switch (RelativeCoordinate::StandardStrings::getTypeOf (symbol))
        {
            case RelativeCoordinate::StandardStrings::x:
            case RelativeCoordinate::StandardStrings::left:     return rect.left.getExpression();
            case RelativeCoordinate::StandardStrings::y:
            case RelativeCoordinate::StandardStrings::top:      return rect.top.getExpression();
            case RelativeCoordinate::StandardStrings::right:    return rect.right.getExpression();
            case RelativeCoordinate::StandardStrings::bottom:   return rect.bottom.getExpression();
            default: break;
        }

        return Expression::Scope::getSymbolValue (symbol);
    }

private:
    const RelativeRectangle& rect;

    JUCE_DECLARE_NON_COPYABLE (RelativeRectangleLocalScope);
};

const Rectangle<float> RelativeRectangle::resolve (const Expression::Scope* scope) const
{
    if (scope == 0)
    {
        RelativeRectangleLocalScope scope (*this);
        return resolve (&scope);
    }
    else
    {
        const double l = left.resolve (scope);
        const double r = right.resolve (scope);
        const double t = top.resolve (scope);
        const double b = bottom.resolve (scope);

        return Rectangle<float> ((float) l, (float) t, (float) jmax (0.0, r - l), (float) jmax (0.0, b - t));
    }
}

void RelativeRectangle::moveToAbsolute (const Rectangle<float>& newPos, const Expression::Scope* scope)
{
    left.moveToAbsolute (newPos.getX(), scope);
    right.moveToAbsolute (newPos.getRight(), scope);
    top.moveToAbsolute (newPos.getY(), scope);
    bottom.moveToAbsolute (newPos.getBottom(), scope);
}

bool RelativeRectangle::isDynamic() const
{
    using namespace RelativeRectangleHelpers;

    return dependsOnSymbolsOtherThanThis (left.getExpression())
            || dependsOnSymbolsOtherThanThis (right.getExpression())
            || dependsOnSymbolsOtherThanThis (top.getExpression())
            || dependsOnSymbolsOtherThanThis (bottom.getExpression());
}

const String RelativeRectangle::toString() const
{
    return left.toString() + ", " + top.toString() + ", " + right.toString() + ", " + bottom.toString();
}

void RelativeRectangle::renameSymbol (const Expression::Symbol& oldSymbol, const String& newName, const Expression::Scope& scope)
{
    left = left.getExpression().withRenamedSymbol (oldSymbol, newName, scope);
    right = right.getExpression().withRenamedSymbol (oldSymbol, newName, scope);
    top = top.getExpression().withRenamedSymbol (oldSymbol, newName, scope);
    bottom = bottom.getExpression().withRenamedSymbol (oldSymbol, newName, scope);
}

//==============================================================================
class RelativeRectangleComponentPositioner  : public RelativeCoordinatePositionerBase
{
public:
    RelativeRectangleComponentPositioner (Component& component_, const RelativeRectangle& rectangle_)
        : RelativeCoordinatePositionerBase (component_),
          rectangle (rectangle_)
    {
    }

    bool registerCoordinates()
    {
        bool ok = addCoordinate (rectangle.left);
        ok = addCoordinate (rectangle.right) && ok;
        ok = addCoordinate (rectangle.top) && ok;
        ok = addCoordinate (rectangle.bottom) && ok;
        return ok;
    }

    bool isUsingRectangle (const RelativeRectangle& other) const throw()
    {
        return rectangle == other;
    }

    void applyToComponentBounds()
    {
        for (int i = 4; --i >= 0;)
        {
            ComponentScope scope (getComponent());
            const Rectangle<int> newBounds (rectangle.resolve (&scope).getSmallestIntegerContainer());

            if (newBounds == getComponent().getBounds())
                return;

            getComponent().setBounds (newBounds);
        }

        jassertfalse; // must be a recursive reference!
    }

    void applyNewBounds (const Rectangle<int>& newBounds)
    {
        if (newBounds != getComponent().getBounds())
        {
            ComponentScope scope (getComponent());
            rectangle.moveToAbsolute (newBounds.toFloat(), &scope);

            applyToComponentBounds();
        }
    }

private:
    RelativeRectangle rectangle;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RelativeRectangleComponentPositioner);
};

void RelativeRectangle::applyToComponent (Component& component) const
{
    if (isDynamic())
    {
        RelativeRectangleComponentPositioner* current = dynamic_cast <RelativeRectangleComponentPositioner*> (component.getPositioner());

        if (current == 0 || ! current->isUsingRectangle (*this))
        {
            RelativeRectangleComponentPositioner* p = new RelativeRectangleComponentPositioner (component, *this);

            component.setPositioner (p);
            p->apply();
        }
    }
    else
    {
        component.setPositioner (0);
        component.setBounds (resolve (0).getSmallestIntegerContainer());
    }
}


END_JUCE_NAMESPACE
