#include <kapp.h>
#include <klocale.h>
#include "kkfmoptdlg.h"

KKFMOptDlg::KKFMOptDlg(QWidget *parent, const char *name, WFlags f)
  : QDialog(parent, name, true, f)
{
  resize(400, 400);
  // i do not want to be resized!
  setMaximumSize(400, 400);
  setMinimumSize(400, 400);
  
  // KTabCtl
  tabDlg = new KTabCtl(this);
  tabDlg->setGeometry(10, 10, 380, 340);
  
  prxDlg = new KProxyDlg(tabDlg);
  tabDlg->addTab(prxDlg, "Proxy");
  usrDlg = new UserAgentDialog(tabDlg);
  tabDlg->addTab(usrDlg, "User Agent");

  // help button
  help = new QPushButton(klocale->translate("Help"), this);
  help->setGeometry(11, 360, 72, 27);
  connect(help, SIGNAL(clicked()), SLOT(helpShow()));

  // ok button
  ok = new QPushButton(klocale->translate("OK"), this);
  ok->setGeometry(230, 360, 72, 27);
  ok->setDefault(TRUE);
  connect(ok, SIGNAL(clicked()), SLOT(accept()));
  
  // cancel button
  cancel = new QPushButton(klocale->translate("Cancel"), this);
  cancel->setGeometry(318, 360, 72, 27);
  connect(cancel, SIGNAL(clicked()), SLOT(reject()));

  // my name is
  setCaption(klocale->translate("KFM Browser Settings"));
}

KKFMOptDlg::~KKFMOptDlg()
{
  // now delete everything we allocated before
  delete help;
  delete ok;
  delete cancel;
  delete usrDlg;
  delete prxDlg;
  delete tabDlg;
}

void KKFMOptDlg::helpShow()
{
  kapp->invokeHTMLHelp(0, 0);
}

void KKFMOptDlg::setUsrAgentData(QStrList *strList)
// forward data to UserAgentDialog
{
  usrDlg->setData(strList);
}

QStrList KKFMOptDlg::dataUsrAgent() const
{
  return(usrDlg->data()); 
}

void KKFMOptDlg::setProxyData(QStrList *strList)
// forward data to KProxyDlg
{
  prxDlg->setData(strList);
}

QStrList KKFMOptDlg::dataProxy() const
{
  return(prxDlg->data());
}

#include "kkfmoptdlg.moc"
