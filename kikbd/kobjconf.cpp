#include <stream.h>
#include <qfileinf.h>
#include <qchkbox.h>
#include <qcombo.h>
#include <qbttngrp.h>
#include <qpushbt.h>
#include <qradiobt.h>

#include <kapp.h>
#include "kobjconf.h"
#include "kconfobjs.h"
#include <kcolordlg.h>
#include <kfontdialog.h>
#include <kcolorbtn.h>

/********************************************************************
 * Configuration objects
 */
KConfigObject::KConfigObject(void* pData, bool pDeleteData, const char* key)
{
  group = 0L;
  data  = pData;
  deleteData = pDeleteData;
  keys.setAutoDelete(TRUE);
  if(key) keys.append(key);
  configure();
}
KConfigObject::~KConfigObject()
{
  if(deleteData) delete data;
}
/**
   regexp matched keys Config Object
*/
KConfigMatchKeysObject::KConfigMatchKeysObject(const QRegExp& match,
					       QStrList& list)
  :KConfigObject(&list, FALSE), regexp(match)
{
}
void KConfigMatchKeysObject::readObject(KObjectConfig* config)
{
  QStrList &list = *((QStrList*)data);
  list.clear();
  keys.clear();
  KEntryIterator* it = config->entryIterator(config->group());
  if(it) {
    for(it->toFirst(); it->current(); ++(*it)) {
      if(regexp.match(it->currentKey()) != -1)
	if(it->current()->aValue) {
	  keys.append(it->currentKey());
	  list.append(it->current()->aValue);
	}
    }
  }
}
void KConfigMatchKeysObject::writeObject(KObjectConfig* config)
{
  QStrList &list = *((QStrList*)data);
  unsigned i;for(i=0; i<keys.count(); i++)
    config->writeEntry(keys.at(i), list.at(i));
}
QStrList KConfigMatchKeysObject::separate(const char* s, char sep=',')
{
  QString  string(s);
  QStrList list;
  int i, j;for(i=0;j=string.find(sep, i), j != -1; i=j+1)
    list.append(string.mid(i, j-i));
  QString last = string.mid(i, 1000);
  if(!last.isNull() && last != "") list.append(last);
  return list;
}
/*
 * numbered keys Config Object
 */
KConfigNumberedKeysObject::KConfigNumberedKeysObject(const char* pKeybase,
						     unsigned pFrom,
						     unsigned pTo,
						     QStrList& list)
  :KConfigObject(&list, FALSE), keybase(pKeybase)
{
  from = pFrom, to = pTo;
}
void KConfigNumberedKeysObject::readObject(KObjectConfig* config)
{
  QStrList &list = *((QStrList*)data);
  list.clear();
  keys.clear();
  unsigned i;for(i=from; i<to; i++) {
    QString num;
    QString key = keybase + num.setNum(i);
    QString entry = config->readEntry(key);
    if(entry.isNull() || entry == "") break;
    keys.append(key);
    list.append(entry);
  }
}
void KConfigNumberedKeysObject::writeObject(KObjectConfig* config)
{
  QStrList &list = *((QStrList*)data);
  unsigned i;for(i=0; i<=list.count(); i++) {
    QString num;
    QString key = keybase + num.setNum(i);
    config->writeEntry(key, i>=list.count()?"":list.at(i));
  }
}
/*
 * Bool Object
 */
void KConfigBoolObject::readObject(KObjectConfig* config)
{
  QString entry = config->readEntry(keys.current());
  if(!entry.isNull()) {
    entry.lower(); entry.stripWhiteSpace();
    *((bool*)data) = (entry == "yes" | entry == "true"
		      | entry == klocale->translate("yes") 
		      | entry == klocale->translate("true"))?TRUE:FALSE;
  }
}
void KConfigBoolObject::writeObject(KObjectConfig* config)
{
  config->writeEntry(keys.current(), (*((bool*)data))?"yes":"no");
}
void KConfigBoolObject::toggled(bool val)
{
  *((bool*)data) = val;
}
QWidget* KConfigBoolObject::createWidget(QWidget* parent, const char* label)
{
  QCheckBox* box = new QCheckBox(label, parent);
  box->setChecked(*((bool*)data));
  box->setMinimumSize(box->sizeHint());
  connect(box, SIGNAL(toggled(bool)), SLOT(toggled(bool)));
  return box;
}
/*
 * Int Object
 */
void KConfigIntObject::readObject(KObjectConfig* config)
{
  *((int*)data) = config->readNumEntry(keys.current(), *((int*)data));
}
void KConfigIntObject::writeObject(KObjectConfig* config)
{
  config->writeEntry(keys.current(), *((int*)data));
}
/*
 * String Object
 */
void KConfigStringObject::readObject(KObjectConfig* config)
{
  *((QString*)data) = config->readEntry(keys.current(), *((QString*)data));
}
void KConfigStringObject::writeObject(KObjectConfig* config)
{
  config->writeEntry(keys.current(), *((QString*)data));
}
/*
 * String List Object
 */
void KConfigStrListObject::readObject(KObjectConfig* config)
{
  config->readListEntry(keys.current(), *((QStrList*)data), sep);
}
void KConfigStrListObject::writeObject(KObjectConfig* config)
{
  config->writeEntry(keys.current(), *((QStrList*)data), sep);
}
/*
 * Combo Object
 */
KConfigComboObject::KConfigComboObject(const char* key, QString& val,
				       const char** list,
				       unsigned num)
  :KConfigStringObject(key, val)
{
  unsigned i;for(i=0; i<num; combo.append(list[i++]));
}
QWidget* KConfigComboObject::createWidget(QWidget* parent,
					  const char** list)
{
  QComboBox* box = new QComboBox(parent);
  unsigned i, j;for(i=j=0; i<combo.count(); i++) {
    if(*((QString*)data) == combo.at(i)) j = i;
    if(list && list[i]) box->insertItem(klocale->translate(list[i]));
    else box->insertItem(combo.at(i));
  }
  box->setCurrentItem(j);
  box->setMinimumSize(box->sizeHint());
  connect(box, SIGNAL(activated(int)), SLOT(activated(int)));
  return box;
}
QWidget* KConfigComboObject::createWidget2(QWidget* parent,
					   const char** list,
					   const char* name)
{
  QButtonGroup* box = new QButtonGroup(name, parent);
  int height = 0;
  unsigned i, j;for(i=j=0; i<combo.count(); i++) {
    if(*((QString*)data) == combo.at(i)) j = i;
    QRadioButton *but = new QRadioButton((list && list[i])?klocale->translate(list[i])
					 :combo.at(i), box);
    but->setMinimumSize(but->sizeHint());
    height = but->height();
  }
  ((QRadioButton*)box->find(j))->setChecked(TRUE);
  box->setMinimumHeight(2*height);
  connect(box, SIGNAL(clicked(int)), SLOT(activated(int)));
  return box;
}
/*
 * Color List Object
 */
void KConfigColorObject::readObject(KObjectConfig* config)
{
  *((QColor*)data) = config->readColorEntry(keys.current(),
					    ((QColor*)data));
}
void KConfigColorObject::writeObject(KObjectConfig* config)
{
  config->writeEntry(keys.current(), *((QColor*)data));
}
QWidget* KConfigColorObject::createWidget(QWidget* parent)
{
  KColorButton *button = new KColorButton(*((const QColor*)data),
					  parent);
  connect(button, SIGNAL(changed(const QColor&)),
	  SLOT(changed(const QColor&)));
  return button;
}
void KConfigColorObject::changed(const QColor& newColor)
{
  *((QColor*)data) = newColor;
}
/*
 * Font Object
 */
void KConfigFontObject::readObject(KObjectConfig* config)
{
  *((QFont*)data) = config->readFontEntry(keys.current(),
					  ((QFont*)data));
}
void KConfigFontObject::writeObject(KObjectConfig* config)
{
  config->writeEntry(keys.current(), *((QFont*)data));
}
QWidget* KConfigFontObject::createWidget(QWidget* parent)
{
  QPushButton *button = new QPushButton(parent);
  connect(button, SIGNAL(clicked()), SLOT(activated()));
  return button;
}
void KConfigFontObject::activated()
{
  QFont font = *((QFont*)data);
  if(KFontDialog::getFont(font)) {
    *((QFont*)data) = font;
  }
}



/*************************************************************************
   Object Configuration 
   

**************************************************************************/
KObjectConfig::KObjectConfig(const char* pGlobalFile, const char* pLocalFile)
  :KConfig(pGlobalFile, pLocalFile)
{
  groups.setAutoDelete(TRUE);
  entries.setAutoDelete(TRUE);

  QFileInfo info(pLocalFile);
  emptyLocal = (!info.exists() || (info.size() == 0));
}
/*
 * set current configuration group
 */
void KObjectConfig::setGroup(const char* pGroup)
{
  if(groups.find(pGroup) == -1) groups.append(pGroup);
}
void KObjectConfig::registerObject(KConfigObject* obj)
{
  obj->group = groups.current();
  entries.append(obj);
}
void KObjectConfig::loadConfig()
{
  unsigned i;for(i=0; i<groups.count(); i++) {
    KConfig::setGroup(groups.at(i));
    unsigned j;for(j=0; j<entries.count(); j++)
      if(groups.at(i) == entries.at(j)->group) {
	entries.at(j)->readObject(this);
      }
  }
  if(emptyLocal) {
    emptyLocal = FALSE;
    emptyLocalConfig();
  }
}
void KObjectConfig::saveConfig()
{
  unsigned i;for(i=0; i<groups.count(); i++) {
    KConfig::setGroup(groups.at(i));
    unsigned j;for(j=0; j<entries.count(); j++)
      if(groups.at(i) == entries.at(j)->group)
	entries.at(j)->writeObject(this);
  }
  sync();
}
KConfigObject* KObjectConfig::find(void* data)
{
  unsigned i;for(i=0; i<entries.count(); i++)
    if(entries.at(i)->data == data)
      return entries.at(i);
  return 0L;
}
/**
   supported in core objects
*/
void KObjectConfig::registerBool(const char* key, bool& val)
{
  registerObject(new KConfigBoolObject(key, val));
}
void KObjectConfig::registerInt(const char* key, int& val)
{
  registerObject(new KConfigIntObject(key, val));
}
void KObjectConfig::registerString(const char* key, QString& val)
{
  registerObject(new KConfigStringObject(key, val));
}
void KObjectConfig::registerStrList(const char* key, QStrList& val,
				    char pSep)
{
  registerObject(new KConfigStrListObject(key, val, pSep));
}
void KObjectConfig::registerColor(const char* key, QColor& val)
{
  registerObject(new KConfigColorObject(key, val));
}
void KObjectConfig::registerFont(const char* key, QFont& val)
{
  registerObject(new KConfigFontObject(key, val));
}
/**
   supported widgets
*/
QWidget* KObjectConfig::createBoolWidget(bool& val, const char* label,
					 QWidget* parent)
{
  KConfigObject *obj = find(&val);
  if(obj)
    return ((KConfigBoolObject*)obj)->createWidget(parent, label);
  return 0L;
}
QWidget* KObjectConfig::createComboWidget(QString& val, const char** list,
					  QWidget* parent)
{
  KConfigObject *obj = find(&val);
  if(obj)
    return ((KConfigComboObject*)obj)->createWidget(parent, list);
  return 0L;
}
QWidget* KObjectConfig::createComboWidget2(QString& val, const char** list,
					   const char* name, QWidget* parent)
{
  KConfigObject *obj = find(&val);
  if(obj)
    return ((KConfigComboObject*)obj)->createWidget2(parent, list, name);
  return 0L;
}
QWidget* KObjectConfig::createColorWidget(QColor& val, QWidget* parent)
{
  KConfigObject *obj = find(&val);
  if(obj)
    return ((KConfigColorObject*)obj)->createWidget(parent);
  return 0L;
}
QWidget* KObjectConfig::createFontWidget(QFont& val, QWidget* parent)
{
  KConfigObject *obj = find(&val);
  if(obj)
    return ((KConfigFontObject*)obj)->createWidget(parent);
  return 0L;
}