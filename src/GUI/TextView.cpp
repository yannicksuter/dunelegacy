/*
 *  This file is part of Dune Legacy.
 *
 *  Dune Legacy is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Dune Legacy is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Dune Legacy.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <GUI/TextView.h>

#include <algorithm>

TextView::TextView() : Widget() {
    fontID = FONT_STD12;
    textcolor = COLOR_DEFAULT;
    textshadowcolor = COLOR_DEFAULT;
    backgroundcolor = COLOR_TRANSPARENT;
    alignment = (Alignment_Enum) (Alignment_Left | Alignment_Top);
    pForeground = nullptr;
    pBackground = nullptr;
    bAutohideScrollbar = true;
    enableResizing(true,true);

    resize(getMinimumSize().x,getMinimumSize().y);
}

TextView::~TextView() {
    invalidateTextures();
}

void TextView::handleMouseMovement(Sint32 x, Sint32 y, bool insideOverlay) {
    scrollbar.handleMouseMovement(x - getSize().x + scrollbar.getSize().x,y,insideOverlay);
}

bool TextView::handleMouseLeft(Sint32 x, Sint32 y, bool pressed) {
    return scrollbar.handleMouseLeft(x - getSize().x + scrollbar.getSize().x,y,pressed);
}

bool TextView::handleMouseWheel(Sint32 x, Sint32 y, bool up)  {
    // forward mouse wheel event to scrollbar
    return scrollbar.handleMouseWheel(0,0,up);
}

bool TextView::handleKeyPress(SDL_KeyboardEvent& key) {
    Widget::handleKeyPress(key);

    scrollbar.handleKeyPress(key);
    return true;
}

void TextView::draw(Point position) {
    if(isVisible() == false) {
        return;
    }

    updateTextures();

    if(pBackground != nullptr) {
        SDL_Rect dest = calcDrawingRect(pBackground, position.x, position.y);
        SDL_RenderCopy(renderer, pBackground, nullptr, &dest);
    }

    if(pForeground != nullptr) {
        int lineHeight = GUIStyle::getInstance().getTextHeight(fontID) + 2;

        SDL_Rect src = {    0,
                            scrollbar.getCurrentValue() * lineHeight,
                            getWidth(pForeground),
                            std::min(getHeight(pForeground), getSize().y - 2) };

        SDL_Rect dest = {   position.x + 2,
                            position.y + 1,
                            getWidth(pForeground),
                            std::min(getHeight(pForeground), getSize().y - 2) };
        SDL_RenderCopy(renderer, pForeground, &src, &dest);
    }

    Point scrollBarPos = position;
    scrollBarPos.x += getSize().x - scrollbar.getSize().x;

    if(!bAutohideScrollbar || (scrollbar.getRangeMin() != scrollbar.getRangeMax())) {
        scrollbar.draw(scrollBarPos);
    }
}

void TextView::resize(Uint32 width, Uint32 height) {
    invalidateTextures();

    scrollbar.resize(scrollbar.getMinimumSize().x,height);

    std::list<std::string> textLines = calcTextLines();

    int lineHeight = GUIStyle::getInstance().getTextHeight(fontID) + 2;

    int numVisibleLines = height/lineHeight;
    scrollbar.setRange(0,std::max(0, ((int) textLines.size()) - numVisibleLines));
    scrollbar.setBigStepSize(std::max(1, numVisibleLines-1));

    Widget::resize(width,height);
}

void TextView::updateTextures() {
    if(pBackground == nullptr) {
        pBackground = convertSurfaceToTexture(GUIStyle::getInstance().createWidgetBackground(getSize().x, getSize().y), true);
    }

    if(pForeground == nullptr) {
        std::list<std::string> textLines = calcTextLines();

        int lineHeight = GUIStyle::getInstance().getTextHeight(fontID) + 2;
        int labelHeight = lineHeight * textLines.size() + 2;
        pForeground = convertSurfaceToTexture(GUIStyle::getInstance().createLabelSurface(getSize().x-4,labelHeight,textLines,fontID,alignment,textcolor,textshadowcolor,backgroundcolor), true);
    }
}

std::list<std::string> TextView::calcTextLines() {
    //split text into single lines at every '\n'
    size_t startpos = 0;
    size_t nextpos;
    std::list<std::string> hardLines;
    do {
        nextpos = text.find("\n",startpos);
        if(nextpos == std::string::npos) {
            hardLines.push_back(text.substr(startpos,text.length()-startpos));
        } else {
            hardLines.push_back(text.substr(startpos,nextpos-startpos));
            startpos = nextpos+1;
        }
    } while(nextpos != std::string::npos);

    std::list<std::string> textLines;

    std::list<std::string>::const_iterator iter;
    for(iter = hardLines.begin();iter != hardLines.end();++iter) {
        std::string tmpLine = *iter;

        if(tmpLine == "") {
            textLines.push_back(" ");
            continue;
        }

        bool EndOfLine = false;
        size_t warppos = 0;
        size_t oldwarppos = 0;
        size_t lastwarp = 0;

        while(EndOfLine == false) {
            while(true) {
                warppos = tmpLine.find(" ", oldwarppos);
                std::string tmp;
                if(warppos == std::string::npos) {
                    tmp = tmpLine.substr(lastwarp,tmpLine.length()-lastwarp);
                    warppos = tmpLine.length();
                    EndOfLine = true;
                } else {
                    tmp = tmpLine.substr(lastwarp,warppos-lastwarp);
                }

                if( GUIStyle::getInstance().getMinimumLabelSize(tmp, fontID).x - 4 > getSize().x - scrollbar.getSize().x - 4) {
                    // this line would be too big => in oldwarppos is the last correct warp pos
                    EndOfLine = false;
                    break;
                } else {
                    if(EndOfLine == true) {
                        oldwarppos = warppos;
                        break;
                    } else {
                        oldwarppos = warppos + 1;
                    }
                }
            }

            if(oldwarppos == lastwarp) {
                // the width of this label is too small for the next word
                // split the word

                warppos = lastwarp;
                while(true) {
                    std::string tmp = tmpLine.substr(lastwarp,warppos-lastwarp);
                    if( GUIStyle::getInstance().getMinimumLabelSize(tmp, fontID).x - 4 > getSize().x - scrollbar.getSize().x - 4) {
                        // this line would be too big => in oldwarppos is the last correct warp pos
                        break;
                    } else {
                        oldwarppos = warppos;
                    }

                    warppos++;
                }

                if(warppos != lastwarp) {
                    textLines.push_back(tmpLine.substr(lastwarp,oldwarppos-lastwarp));
                    lastwarp = oldwarppos;
                } else {
                    // the width of this label is too small for the next character
                    // create a dummy entry
                    textLines.push_back(" ");
                    lastwarp++;
                    oldwarppos++;
                }
            } else {
                std::string tmpStr = tmpLine.substr(lastwarp,oldwarppos-lastwarp);
                textLines.push_back(tmpStr);
                lastwarp = oldwarppos;
            }
        }
    }

    return textLines;
}
