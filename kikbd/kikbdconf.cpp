/*
   - 

  written 1998 by Alexander Budnik <budnik@linserv.jinr.ru>
  
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
   
  */
#include <stream.h>
#include <stdlib.h>
#include <qfileinf.h>
#include <qdir.h>
#include <qcolor.h>
#include <qpainter.h>

#include <kconfig.h>
#include <kapp.h>
#include <kmsgbox.h>
#include <kiconloader.h> 

#include "kikbdconf.h"
#include "kconfobjs.h"

//=========================================================
//    data
//=========================================================
static const char* confMainGroup        = "International Keyboard";
static const char* confStartupGroup     = "StartUp";
static const char* confMapsGroup        = "KeyboardMap";

static const char* confStringBeep       = "Beep";
static const char* confStringSwitch     = "Switch";
static const char* confStringAltSwitch  = "AltSwitch";
static const char* confStringAutoMenu   = "WorldMenu";
static const char* confStringEmuCapsLock= "EmulateCapsLock";
static const char* confStringSaveClasses= "SaveClasses";
static const char* confStringInput      = "Input";
static const char* confStringMap        = "Map";
static const char* confStringLabel      = "Label";
static const char* confStringComment    = "Comment";
static const char* confStringLanguage   = "Language";
static const char* confStringLocale     = "Locale";
static const char* confStringCharset    = "Charset";
static const char* confStringAuthors    = "Authors";
static const char* confStringCaps       = "CapsSymbols";
static const char* confStringCapsColor  = "CapsLockColor";
static const char* confStringAltColor   = "AltColor";
static const char* confStringForColor   = "ForegroundColor";
static const char* confStringFont       = "Font";
static const char* confStringCustFont   = "CustomizeFont";
static const char* confStringAutoStart  = "AutoStart";
static const char* confStringDocking    = "Docking";  
static const char* confStringHotList    = "HotList";  
static const char* confStringAutoStartPlace  = "AutoStartPlace";
static const char* swConfigString[] = {
  "None",
  "Alt_R",
  "Control_R",
  "Alt_R+Shift_R",
  "Control_R+Alt_R",
  "Control_R+Shift_R",
  "Alt_L+Shift_L",
  "Control_L+Alt_L",
  "Control_L+Shift_L",
  "Shift_L+Shift_R"
};
static const char* swConfigAltString[] = {
  "None",
  "Alt_R",
  "Control_R",
  "Alt_L",
  "Control_L"
};
static const char* inpConfigString[] = {
  "Global", "Window", "Class"
};

const char* switchLabels[] = {
  "(None)",
  "Right Alt",
  "Right Control",
  "Rights (Alt+Shift)" ,
  "Rights (Ctrl+Alt)"  ,
  "Rights (Ctrl+Shift)",
  "Lefts  (Alt+Shift)",
  "Lefts  (Ctrl+Alt)",
  "Lefts  (Ctrl+Shift)",
  "Both Shift's (Shift+Shift)"
};
const char* altSwitchLabels[] = {
  "(None)",
  "Right Alt",
  "Right Control",
  "Left  Alt",
  "Left  Control",
};
const char* autoStartPlaceLabels[] = {
  "Top Left",
  "Top Right",
  "Bottom Left",
  "Bottom Right"
};

const QColor mapNormalColor = black;
const QColor mapUserColor   = darkBlue;
const QColor mapNoFileColor = darkRed;

//=========================================================
//   config class
//=========================================================
KiKbdConfig::KiKbdConfig():KObjectConfig(UserFromSystemRc)
{
  setVersion(1.0);
  *this << setGroup(confMainGroup)
	<< new KConfigBoolObject(confStringHotList, hotList)
	<< new KConfigBoolObject(confStringBeep, keyboardBeep)
	<< new KConfigBoolObject(confStringAutoMenu, autoMenu)
	<< new KConfigBoolObject(confStringEmuCapsLock, emuCapsLock)
	<< new KConfigBoolObject(confStringCustFont, custFont)
	<< new KConfigBoolObject(confStringSaveClasses, saveClasses)
	<< new KConfigNumberedKeysObject(confStringMap, 0, 9, maps)
	<< new KConfigComboObject(confStringSwitch, switchComb, swConfigString,
				  sizeof(swConfigString)
				  /sizeof(*swConfigString), switchLabels)
	<< new KConfigComboObject(confStringAltSwitch, altSwitchComb,
				  swConfigAltString, sizeof(swConfigAltString)
				  /sizeof(*swConfigAltString), altSwitchLabels)
	<< new KConfigComboObject(confStringInput, input, inpConfigString,
				  sizeof(inpConfigString)
				  /sizeof(*inpConfigString), inpConfigString,
				  KConfigComboObject::ButtonGroup)
	<< new KConfigColorObject(confStringCapsColor, capsColor)
	<< new KConfigColorObject(confStringAltColor, altColor)
	<< new KConfigColorObject(confStringForColor, forColor)
	<< new KConfigFontObject(confStringFont,  font)
	<< setGroup(confStartupGroup)
	<< new KConfigBoolObject(confStringAutoStart, autoStart)
	<< new KConfigBoolObject(confStringDocking, docking)
	<< new KConfigComboObject(confStringAutoStartPlace, autoStartPlace,
				  autoStartPlaceLabels,
				  sizeof(autoStartPlaceLabels)
				  /sizeof(*autoStartPlaceLabels));
  connect(this, SIGNAL(newUserRcFile()), SLOT(newUserRc())); 
  connect(this, SIGNAL(olderVersion()) , SLOT(olderVersion())); 
  connect(this, SIGNAL(newerVersion()) , SLOT(newerVersion())); 
  maps.setAutoDelete(TRUE);
  allMaps.setAutoDelete(TRUE);
}
void KiKbdConfig::loadConfig()
{
  // default values
  keyboardBeep = hotList = autoStart = docking = TRUE;
  autoMenu = emuCapsLock = custFont = saveClasses = FALSE;
  switchComb = altSwitchComb = autoStartPlace = "";
  input = 0;
  capsColor = QColor(0, 128, 128);
  altColor  = QColor(255, 255, 0);
  forColor  = black;

  KObjectConfig::loadConfig();
}
void ask(const char* msg)
{
  KiKbdConfig::message(translate(msg));
  if(QString(kapp->argv()[0]).find("kikbd") != -1) {
    if(KMsgBox::yesNo(0L, "kikbd",
		      translate("Do you want to start Configuration?"))
       == 1) {
      system("kcmikbd -startkikbd&");
      ::exit(0);
    }
  }
}
void KiKbdConfig::newUserRc()
{
  ask("Your configuration is empty. Install system default.");
}
int doneCheck = FALSE;
void KiKbdConfig::olderVersion()
{
  if(!doneCheck) {
    ask("Configuration file you have has older format when expected.\n"
	"Some settings may be incorrect.");
    doneCheck = TRUE;
  }
}
void KiKbdConfig::newerVersion()
{
  if(!doneCheck) {
    ask("Configuration file you have has newer format when expected.\n"
	"Some settings may be incorrect.");
    doneCheck = TRUE;
  }
}
QStrList KiKbdConfig::availableMaps()
{
  QStrList list, dirs;
  dirs.append(kapp->kde_datadir() + "/kikbd");
  dirs.append(kapp->localkdedir() + "/share/apps/kikbd");
  unsigned i;for(i=0; i<dirs.count(); i++)
    {
      QDir dir(dirs.at(i));  
      if(!dir.exists()) continue;
      QStrList entry = *dir.entryList("*.kimap",
				     QDir::Files | QDir::Readable,
				     QDir::Name | QDir::IgnoreCase);
      unsigned j;for(j=0; j<entry.count(); j++)
	{
	  QString name = entry.at(j);
	  name.resize(name.find(".")+1);
	  if(list.find(name) == -1) list.inSort(name);
	}
    }
  return list;
}
KiKbdMapConfig* KiKbdConfig::getMap(const char* name)
{
  unsigned i;for(i=0; i<allMaps.count(); i++)
    if(allMaps.at(i)->name == name) return allMaps.at(i);
  allMaps.append(new KiKbdMapConfig(name));
  return allMaps.current();
}
bool KiKbdConfig::hasAltKeys()
{
  unsigned i;for(i=0; i<maps.count(); i++)
    if(getMap(maps.at(i))->getHasAltKeys()) return TRUE;
  return FALSE;
}
bool KiKbdConfig::oneKeySwitch()
{
  return (!switchComb.contains('+')) && (switchComb != "None");
}
void KiKbdConfig::error(const char* form, const char* str,
			const char* str2)
{
  QString msg(128);
  msg.sprintf(form, str, str2);
  if(KMsgBox::yesNo(0, translate("kikbd configuration error"), msg) == 2) 
    ::exit(1);
}
void KiKbdConfig::message(const char* form, const char* str)
{
  QString msg(128);
  msg.sprintf(form, str);
  KMsgBox::message(0L, translate("kikbd configuration message"), msg);
}

//=========================================================
// map configuration
//=========================================================
KiKbdMapConfig::KiKbdMapConfig(const char* nm):name(nm)
{
  KObjectConfig config(KObjectConfig::AppData, name + ".kimap");
  QStrList symList, codeList;
  config << config.setGroup(confMainGroup)
	 << new KConfigStringObject(confStringLabel, label)
	 << new KConfigStringObject(confStringComment, comment)
	 << new KConfigStringObject(confStringLanguage, language)
	 << new KConfigStringObject(confStringCharset, charset)
	 << new KConfigStringObject(confStringLocale, locale)
	 << new KConfigStrListObject(confStringAuthors , authors)
	 << config.setGroup(confMapsGroup)
	 << new KConfigMatchKeysObject(QRegExp("^keysym[0-9]+$"), symList)
	 << new KConfigMatchKeysObject(QRegExp("^keycode[0-9]+$"), codeList)
	 << new KConfigStrListObject(confStringCaps, capssyms);
  userData = TRUE;
  noFile   = FALSE;
  connect(&config, SIGNAL(noUserDataFile(const char*)),
	  SLOT(noUserDataFile(const char*)));
  connect(&config, SIGNAL(noSystemDataFile(const char*)),
	  SLOT(noSystemDataFile(const char*)));
  config.loadConfig();
  /*--- check information ---*/
  if(comment.isNull() ) comment  = "";
  else {
    if(comment[comment.length()-1] != '.') comment += ".";
  }
  if(language.isNull()) language = "unknown";
  if(charset.isEmpty()) charset  = "unknown";
  if(label.isNull() || label == "")
    if(!locale.isNull()) label = locale; else label = name;
  if(locale.isNull() || locale == "") locale = translate("default");
  /*--- parsing ---*/
  keysyms.setAutoDelete(TRUE);
  keycodes.setAutoDelete(TRUE);
  capssyms.setAutoDelete(TRUE);
  hasAltKeys = FALSE;
  /*--- pars key symbols ---*/
  unsigned i;for(i=0; i<symList.count(); i++) {
    QStrList *map = new QStrList;
    *map = KObjectConfig::separate(symList.at(i));
    keysyms.append(map);
    if(!hasAltKeys && map->count() > 3) hasAltKeys = TRUE;
  }
  /*--- pars key codes ---*/
  for(i=0; i<codeList.count(); i++) {
    QStrList *map = new QStrList;
    *map = KObjectConfig::separate(codeList.at(i));
    keycodes.append(map);
    if(!hasAltKeys && map->count() > 3) hasAltKeys = TRUE;
  }
}
const QString KiKbdMapConfig::getInfo() const
{
  QStrList authors(this->authors);
  QString com;
  // authors
  com = translate(authors.count()<2?"Author":"Authors");
  com += ":  ";
  com += authors.count()>0?authors.at(0):translate("Not specified");
  for(unsigned i = 1; i < authors.count() && i < 4; i++) {
    com += ",  ";
    com += authors.at(i);
  }
  com += "\n\n";
  // description
  com += translate("Description:  ") + getGoodLabel()
    + " " + comment;
  com += "\n\n";
  // source
  com += translate("Source:  ");
  com += noFile?translate("no file")
    :(userData?translate("user file")
    :translate("system file"));
  com += " \"" + name + ".kimap\"";
  com += "\n";
  // statistic
  QString num;
  com += translate("Statistic:  ");
  num.setNum(keysyms.count());
  com += num + " ";
  com += translate("symbols");
  num.setNum(keycodes.count());
  com += ", " + num + " ";
  com += translate("codes");
  if(hasAltKeys) {
    com += ", ";
    com += translate("alternative symbols");
  }
  return com;
}
const QString KiKbdMapConfig::getGoodLabel() const
{
  QString label;
  label += language + " ";
  label += translate("language");
  label += ", " + charset + " ";
  label += translate("charset");
  label += ".";
  return label;
}
const QColor KiKbdMapConfig::getColor() const
{
  return noFile?mapNoFileColor:(userData?mapUserColor:mapNormalColor);
}
const QPixmap KiKbdMapConfig::getIcon() const
{
  KIconLoader loader;
  QPixmap pm(21, 14);
  QPainter p;

  loader.insertDirectory(0, kapp->kde_datadir()+"/kcmlocale/pics/");
  QPixmap flag(loader.loadIcon(QString("flag_")+locale+".gif", 21, 14));

  pm.fill(white);
  p.begin(&pm);
  p.fillRect(0, 0, 20, 13, gray);
  if(!flag.isNull())
    p.drawPixmap(0, 0, flag);
  p.setPen(black);
  p.drawText(0, 0, 20, 13, AlignCenter, label);
  p.end();

  return pm;
}
