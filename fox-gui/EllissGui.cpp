/******************************************************************************
*                                                                             *
*                                  Elliss Gui                                 *
*                                                                             *
*******************************************************************************
* Copyright (C) 2005,2006 by Francesco Abbate.   All Rights Reserved.         *
*******************************************************************************
* $Id: EllissGui.cpp,v 1.5 2006/12/29 17:47:08 francesco Exp $                       *
******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>

#include <gsl/gsl_vector.h>

#include "EllissGui.h"
#include "DispersDialog.h"
#include "BatchDialog.h"
#include "InteractiveFit.h"
#include "Strcpp.h"
#include "error-messages.h"
#include "fit-engine.h"
#include "multi-fit-engine.h"
#include "lmfit-multi.h"
#include "fit-params.h"
#include "spectra.h"
#include "symtab.h"
#include "grid-search.h"
#include "str.h"
#include "str-util.h"
#include "dispers-library.h"

static float timeval_subtract (struct timeval *x, struct timeval *y);

// Map
FXDEFMAP(EllissWindow) EllissWindowMap[]={
  FXMAPFUNC(SEL_PAINT,   EllissWindow::ID_CANVAS, EllissWindow::onCmdPaint),
  FXMAPFUNC(SEL_UPDATE,  EllissWindow::ID_CANVAS, EllissWindow::onUpdCanvas),
  FXMAPFUNC(SEL_COMMAND, EllissWindow::ID_ABOUT,  EllissWindow::onCmdAbout),
  FXMAPFUNC(SEL_COMMAND, EllissWindow::ID_LOAD_SCRIPT, EllissWindow::onCmdLoadScript),
  FXMAPFUNC(SEL_COMMAND, EllissWindow::ID_SAVE_SCRIPT, EllissWindow::onCmdSaveScript),
  FXMAPFUNC(SEL_COMMAND, EllissWindow::ID_SAVEAS_SCRIPT, EllissWindow::onCmdSaveAsScript),
  FXMAPFUNC(SEL_COMMAND, EllissWindow::ID_LOAD_SPECTRA, EllissWindow::onCmdLoadSpectra),
  FXMAPFUNC(SEL_COMMAND, EllissWindow::ID_DISP_PLOT, EllissWindow::onCmdPlotDispers),
  FXMAPFUNC(SEL_COMMAND, EllissWindow::ID_RUN_FIT, EllissWindow::onCmdRunFit),
  FXMAPFUNC(SEL_COMMAND, EllissWindow::ID_INTERACTIVE_FIT, EllissWindow::onCmdInteractiveFit),
  FXMAPFUNC(SEL_COMMAND, EllissWindow::ID_RUN_MULTI_FIT, EllissWindow::onCmdRunMultiFit),
  FXMAPFUNC(SEL_COMMAND, EllissWindow::ID_RUN_BATCH, EllissWindow::onCmdRunBatch),
  };


// Object implementation
FXIMPLEMENT(EllissWindow,FXMainWindow,EllissWindowMap,ARRAYNUMBER(EllissWindowMap));


const FXchar EllissWindow::patterns_fit[] =
  "Fit Strategy (*.fit)"
  "\nAll Files (*)";
const FXchar EllissWindow::patterns_spectr[] =
  "Fit Strategy (*.dat)"
  "\nAll Files (*)";

const FXHiliteStyle EllissWindow::tstyles[] = {
  {FXRGB(255,255,255), FXRGB(255,0,0), 0, 0, 0, 0, 0, 0}};
  


// Make some windows
EllissWindow::EllissWindow(FXApp* a) 
 : FXMainWindow(a,"Regress Pro",NULL,NULL,DECOR_ALL,20,20,700,460),
   spectrum(NULL), stack_result(NULL), scriptFile("untitled"),
   spectrFile("untitled"), batchFileId("untitled####.dat") {

  // Menubar
  menubar=new FXMenuBar(this,FRAME_RAISED|LAYOUT_SIDE_TOP|LAYOUT_FILL_X);
  statusbar=new FXStatusBar(this,LAYOUT_SIDE_BOTTOM|LAYOUT_FILL_X|FRAME_RAISED|STATUSBAR_WITH_DRAGCORNER);

  // Script menu
  filemenu=new FXMenuPane(this);
  new FXMenuCommand(filemenu,"&Load",NULL,this,ID_LOAD_SCRIPT);
  new FXMenuCommand(filemenu,"&Save",NULL,this,ID_SAVE_SCRIPT);
  new FXMenuCommand(filemenu,"Save As",NULL,this,ID_SAVEAS_SCRIPT);
  new FXMenuCommand(filemenu,"&Quit\tCtl-Q",NULL,getApp(),FXApp::ID_QUIT);
  new FXMenuTitle(menubar,"&Script",NULL,filemenu);

  // Script menu
  spectrmenu = new FXMenuPane(this);
  new FXMenuCommand(spectrmenu,"&Load Spectra",NULL,this,ID_LOAD_SPECTRA);
  new FXMenuTitle(menubar,"S&pectra",NULL,spectrmenu);

  // Dispersion menu
  dispmenu = new FXMenuPane(this);
  new FXMenuCommand(dispmenu, "&Plot Dispersion",NULL,this,ID_DISP_PLOT);
  new FXMenuTitle(menubar,"&Dispersion",NULL,dispmenu);

  // Fit menu
  fitmenu = new FXMenuPane(this);
  new FXMenuCommand(fitmenu, "&Run Fitting",NULL,this,ID_RUN_FIT);
  new FXMenuCommand(fitmenu, "&Interactive Fit",NULL,this,ID_INTERACTIVE_FIT);
  new FXMenuCommand(fitmenu, "Run &Multiple Fit",NULL,this,ID_RUN_MULTI_FIT);
  new FXMenuCommand(fitmenu, "Run &Batch",NULL,this,ID_RUN_BATCH);
  new FXMenuTitle(menubar,"Fittin&g",NULL,fitmenu);

  helpmenu = new FXMenuPane(this);
  new FXMenuCommand(helpmenu, "&About", NULL, this, ID_ABOUT);
  new FXMenuTitle(menubar, "&Help", NULL, helpmenu, LAYOUT_RIGHT);

  // Container
  FXHorizontalFrame *cont = new FXHorizontalFrame(this,LAYOUT_FILL_X|LAYOUT_FILL_Y|FRAME_RAISED);

  tabbook = new FXTabBook(cont,NULL,0,PACK_UNIFORM_WIDTH|PACK_UNIFORM_HEIGHT|LAYOUT_FILL_X|LAYOUT_FILL_Y|LAYOUT_RIGHT);

  // First item is a list
  tabscript = new FXTabItem(tabbook,"&Script",NULL);
  FXHorizontalFrame *lf = new FXHorizontalFrame(tabbook,FRAME_THICK|FRAME_RAISED);
  FXHorizontalFrame *bf = new FXHorizontalFrame(lf,FRAME_THICK|FRAME_SUNKEN|LAYOUT_FILL_X|LAYOUT_FILL_Y, 0,0,0,0, 0,0,0,0);
  scripttext = new FXText(bf,NULL,0,TEXT_WORDWRAP|LAYOUT_FILL_X|LAYOUT_FILL_Y);
  scripttext->setStyled(TRUE);
  scripttext->setHiliteStyles(tstyles);
  scriptfont = new FXFont(getApp(), "Monospace", 10);
  scripttext->setFont(scriptfont);

  new FXTabItem(tabbook,"&Fit Results",NULL);
  lf = new FXHorizontalFrame(tabbook,FRAME_THICK|FRAME_RAISED);
  bf = new FXHorizontalFrame(lf,FRAME_THICK|FRAME_SUNKEN|LAYOUT_FILL_X|LAYOUT_FILL_Y, 0,0,0,0, 0,0,0,0);
  resulttext = new FXText(bf,NULL,0,TEXT_READONLY|TEXT_WORDWRAP|LAYOUT_FILL_X|LAYOUT_FILL_Y);
  resulttext->setFont(scriptfont);

  tabplot = new FXTabItem(tabbook,"&Plot Result",NULL);
  lf = new FXHorizontalFrame(tabbook,FRAME_THICK|FRAME_RAISED);
  bf = new FXHorizontalFrame(lf,FRAME_THICK|FRAME_SUNKEN|LAYOUT_FILL_X|LAYOUT_FILL_Y, 0,0,0,0, 0,0,0,0);
  plotcanvas = new FXCanvas(bf,this,ID_CANVAS,LAYOUT_FILL_X|LAYOUT_FILL_Y);

  plotkind = SYSTEM_UNDEFINED;

  spectrPlot1 = new FXDataPlot();
  spectrPlot2 = new FXDataPlot();

  isPlotModified = true;

  symbol_table_init (this->symtab);

  init_class_list ();

  dispers_library_init ();
}


// Create image
void
EllissWindow::create(){
  FXMainWindow::create();
  scriptfont->create();
  FXDataPlot::initPlotEngine(getApp());
}

void
EllissWindow::plotCanvas(FXDCWindow *dc)
{
  int ww = plotcanvas->getWidth(), hh = plotcanvas->getHeight();

  switch (plotkind)
    {
    case SYSTEM_REFLECTOMETER:
      spectrPlot1->draw(dc, ww, hh);
      break;
    case SYSTEM_ELLISS_AB:
    case SYSTEM_ELLISS_PSIDEL:
      spectrPlot1->draw(dc, ww, hh / 2);
      spectrPlot2->draw(dc, ww, hh / 2, 0, hh / 2);
      break;
    default:
      dc->setForeground(FXRGB(255,255,255));
      dc->fillRectangle(0, 0, ww, hh);
    }
}

// Command from chart
long
EllissWindow::onCmdPaint(FXObject *, FXSelector, void *ptr) {
  if (isPlotModified)
    {
      FXDCWindow dc(plotcanvas);
      plotCanvas(&dc);
    }
  else
    {
      FXDCWindow dc(plotcanvas, (FXEvent*) ptr);
      plotCanvas(&dc);
    }
  isPlotModified = false;
  return 1;
}

long
EllissWindow::onUpdCanvas(FXObject*, FXSelector, void *)
{
  if (isPlotModified)
    {
      FXDCWindow dc(plotcanvas);
      plotCanvas(&dc);
      isPlotModified = false;
      return 1;
    }
  return 0;
}

long
EllissWindow::onCmdLoadScript(FXObject*,FXSelector,void *)
{
  FXFileDialog open(this,"Open Script");
  open.setFilename(scriptFile);
  open.setPatternList(patterns_fit);

  if(open.execute())
    {
      str_t script_file;
      scriptFile = open.getFilename();
      Str script_text;

      if (str_loadfile (scriptFile.text(), script_text.str()) != 0)
	return 0;

      str_init_from_c (script_file, scriptFile.text());
      str_dirname (this->symtab->env->script_dir, script_file, DIR_SEPARATOR);
      str_free (script_file);

      scripttext->setText(script_text.cstr());
      setFitStrategy(script_text.cstr());
      scripttext->setModified(FALSE);
    }

  return 0;
}

FXbool
EllissWindow::saveScriptAs (const FXString& save_as)
{
  FILE *f = fopen (save_as.text(), "w");

  if (f == NULL)
    {
      FXMessageBox::information(this, MBOX_OK, "Script save",
				"Cannot write file %s\n", scriptFile.text());
      return false;
    }

  if (fputs (scripttext->getText().text(), f) == EOF)
    {
      FXMessageBox::information(this, MBOX_OK, "Script save",
				"Cannot write file %s\n", scriptFile.text());
      fclose (f);
      return false;
    }

  fputc ('\n', f);
  fclose (f);
  return true;
  
}

long
EllissWindow::onCmdSaveAsScript(FXObject*,FXSelector,void *)
{
  FXFileDialog open(this, "Save Script As");
  open.setFilename(scriptFile);
  open.setPatternList(patterns_fit);

  if(open.execute())
    {
      FXString new_filename = open.getFilename();
      if (saveScriptAs(new_filename))
	scriptFile = new_filename;
    }

  return 0;
}

long
EllissWindow::onCmdSaveScript(FXObject*,FXSelector,void *)
{
  saveScriptAs(scriptFile);
  return 0;
}

long
EllissWindow::onCmdLoadSpectra(FXObject*,FXSelector,void *)
{
  FXFileDialog open(this,"Open Spectra");
  open.setFilename(spectrFile);
  open.setPatternList(patterns_spectr);

  if(open.execute())
    {
      spectrFile = open.getFilename();

      if (this->spectrum)
	spectra_free (this->spectrum);

      this->spectrum = load_gener_spectrum (spectrFile.text());

      if (this->spectrum == NULL)
	FXMessageBox::information(this, MBOX_OK, "Spectra loading",
				  "Cannot load spectra %s", spectrFile.text());
    }

  return 0;
}

long
EllissWindow::onCmdPlotDispers(FXObject*,FXSelector,void*)
{
  if (this->stack_result)
    {
      DispersDialog dialog(this, this->stack_result);
      dialog.execute();
    }

  return 1;
}

long
EllissWindow::onCmdAbout(FXObject *, FXSelector, void *)
{
  FXDialogBox about(this,"About Regress Pro",DECOR_TITLE|DECOR_BORDER,0,0,0,0,
		    0,0,0,0, 0,0);
  FXVerticalFrame* side=new FXVerticalFrame(&about,LAYOUT_SIDE_RIGHT|LAYOUT_FILL_X|LAYOUT_FILL_Y,0,0,0,0, 10,10,10,10, 0,0);
  new FXLabel(side,"R e g r e s s   P r o",NULL,JUSTIFY_LEFT|ICON_BEFORE_TEXT|LAYOUT_FILL_X);
  new FXHorizontalSeparator(side,SEPARATOR_LINE|LAYOUT_FILL_X);
  new FXLabel(side,FXStringFormat("\nRegress Pro, version %d.%d.%d.\n\n" "Regress Pro is a scientific / industrial software to perform regression\nanalysis of measurement data coming from spectroscopic\nellipsometers or reflectometers.\n" "Regress Pro uses the FOX Toolkit version %d.%d.%d.\nCopyright (C) 2005-2011 Francesco Abbate (francesco.bbt@gmail.com).\n",VERSION_MAJOR,VERSION_MINOR,VERSION_PATCH,FOX_MAJOR,FOX_MINOR,FOX_LEVEL),NULL,JUSTIFY_LEFT|LAYOUT_FILL_X|LAYOUT_FILL_Y);
  FXButton *button=new FXButton(side,"&OK",NULL,&about,FXDialogBox::ID_ACCEPT,BUTTON_INITIAL|BUTTON_DEFAULT|FRAME_RAISED|FRAME_THICK|LAYOUT_RIGHT,0,0,0,0,32,32,2,2);
  button->setFocus();
  about.execute(PLACEMENT_OWNER);
  return 1;
}

// Clean up
EllissWindow::~EllissWindow() {
  FXDataPlot::closePlotEngine();
  delete scriptfont;
  delete filemenu;
  delete spectrmenu;
  delete dispmenu;
  delete fitmenu;
  delete helpmenu;
  delete spectrPlot1;
  delete spectrPlot2;

  if (this->spectrum)
    spectra_free (this->spectrum);

  if (this->stack_result)
    stack_free (this->stack_result);

  symbol_table_clean (this->symtab);
  symbol_table_free  (this->symtab);

  clean_error_msgs ();
}

long
EllissWindow::onCmdRunBatch(FXObject*,FXSelector,void *)
{
  struct seeds *seeds;
  struct fit_engine *fit;

  fit = build_fit_engine (this->symtab, &seeds);

  if (fit == NULL)
    return 0;

  BatchDialog batch(this, fit, seeds);
  batch.setFilename(batchFileId);

  FXString result;
  batch.execute(result);
  resulttext->setText(result);
  resulttext->setModified(TRUE);

  batchFileId = batch.getFilename();

  fit_engine_free (fit);

  return 1;
}

long
EllissWindow::onCmdRunMultiFit(FXObject*,FXSelector,void *)
{
  struct multi_fit_engine *fit;
  Str analysis;
  struct {
    struct seeds *common;
    struct seeds *individual;
  } seeds;

  updateFitStrategy();

  fit = build_multi_fit_engine (this->symtab,
				&seeds.common, &seeds.individual);

  if (fit == NULL)
    return 0;

  Str fit_error_msgs;
  ProgressInfo progress(this->getApp(), this);

  lmfit_multi (fit, seeds.common, seeds.individual,
	       analysis.str(), fit_error_msgs.str(),
	       LMFIT_GET_RESULTING_STACK, 
	       process_foxgui_events, &progress);

  progress.hide();

  if (fit_error_msgs.length() > 0)
    {
      FXMessageBox::information(this, MBOX_OK, "Multiple Fit messages",
				fit_error_msgs.cstr());
      clean_error_msgs ();
    }

  FXString text_fit_result;

  Str fp_results;
  multi_fit_engine_print_fit_results (fit, fp_results.str());
  text_fit_result.append(fp_results.cstr());

  text_fit_result.append(analysis.cstr());

  resulttext->setText(text_fit_result);
  resulttext->setModified(TRUE);

  multi_fit_engine_disable (fit);
  multi_fit_engine_free (fit);

  return 1;
}

void
EllissWindow::updateFitStrategy()
{
  if (scripttext->isModified())
    setFitStrategy (scripttext->getText().text());
}

long
EllissWindow::onCmdRunFit(FXObject*,FXSelector,void *)
{
  if (this->spectrum == NULL)
    {
      FXMessageBox::information(this, MBOX_OK, "Fitting",
				"Please load a spectra before.");
      return 0;
    }

  updateFitStrategy();

  double chisq;
  Str analysis;
  struct seeds *seeds;
  struct fit_engine *fit;

  fit = build_fit_engine (this->symtab, &seeds);

  if (fit == NULL)
    {
      reportErrors();
      return 0;
    }

  fit_engine_prepare (fit, this->spectrum);

  Str fit_error_msgs;
  ProgressInfo progress(this->getApp(), this);

  lmfit_grid (fit, seeds, &chisq, analysis.str(), fit_error_msgs.str(),
	      LMFIT_GET_RESULTING_STACK,
	      process_foxgui_events, & progress);

  progress.hide();

  if (fit_error_msgs.length() > 0)
    {
      FXMessageBox::information(this, MBOX_OK, "Fit messages",
				fit_error_msgs.cstr());
      clean_error_msgs ();
    }

  FXString fitresult, row;

  /* name of the fit script */
  row.format("%s :\n", scriptFile.text());
  fitresult.append(row);

  /* fit parameters results */
  Str fit_parameters_results;
  fit_engine_print_fit_results (fit, fit_parameters_results.str(), 0);
  fitresult.append(fit_parameters_results.cstr());

  /* final chi square obtained */
  row.format("Residual Chisq/pt: %g\n", chisq);
  fitresult.append(row);

  /* covariance matrix analysis */
  fitresult.append("\n");
  fitresult.append(analysis.cstr());

  resulttext->setText(fitresult);
  resulttext->setModified(TRUE);

  fit_engine_restore_spectr (fit);

  this->plotkind = fit->system_kind;

  struct spectrum *gensp = generate_spectrum (fit);
  XYDataSet eds, gds;
  switch (fit->system_kind)
    {
    case SYSTEM_REFLECTOMETER:
      eds = XYDataSet(fit->spectr->table, 0, 1);
      gds = XYDataSet(gensp->table, 0, 1);

      spectrPlot1->clear();
      spectrPlot1->addPlot(eds, FXRGB(255,0,0));
      spectrPlot1->addPlot(gds);
      spectrPlot1->setTitle("Reflectivity");
      break;
    case SYSTEM_ELLISS_AB:
    case SYSTEM_ELLISS_PSIDEL:
      eds = XYDataSet(fit->spectr->table, 0, 1);
      gds = XYDataSet(gensp->table, 0, 1);

      spectrPlot1->clear();
      spectrPlot1->addPlot(eds, FXRGB(255,0,0));
      spectrPlot1->addPlot(gds);
      spectrPlot1->setTitle(fit->system_kind == SYSTEM_ELLISS_AB ? \
			    "SE alpha" : "Tan(Psi)");

      eds = XYDataSet(fit->spectr->table, 0, 2);
      gds = XYDataSet(gensp->table, 0, 2);

      spectrPlot2->clear();
      spectrPlot2->addPlot(eds, FXRGB(255,0,0));
      spectrPlot2->addPlot(gds);
      spectrPlot2->setTitle(fit->system_kind == SYSTEM_ELLISS_AB ? \
			    "SE beta" : "Cos(Delta)");
      break;
    default:
      /* */;
    }

  spectra_free (gensp);

  if (this->stack_result)
    stack_free (this->stack_result);

  this->stack_result = stack_copy (fit->stack);

  fit_engine_disable (fit);
  fit_engine_free (fit);

  isPlotModified = true;

  getApp()->endWaitCursor();

  return 1;
}

long
EllissWindow::onCmdInteractiveFit(FXObject*,FXSelector,void*)
{
  InteractiveFit *fitwin = new InteractiveFit(this, this->symtab);
  fitwin->create();
  fitwin->show();
  return 1;
}

FXbool
EllissWindow::setFitStrategy(const char *script_text)
{
  cleanScriptErrors();

  if (parse_strategy (this->symtab, script_text) != 0)
    {
      int fline, lline;
      str_t errmsg;
      str_init (errmsg, 128);
      
      if (get_script_error_region (&fline, &lline) == 0)
	setErrorRegion(fline, lline);

      get_errors_list (errmsg);

      FXMessageBox::information(this, MBOX_OK, "Script parsing",
				"The parsing of the script has been"
				" unsuccessful :\n%s", CSTR(errmsg));

      str_free (errmsg);
      return false;
    }

  return true;
}

void
EllissWindow::reportErrors()
{
  str_t errmsg;
  str_init (errmsg, 128);
  get_errors_list (errmsg);
  FXMessageBox::information(this, MBOX_OK, "Script parsing",
			    "The parsing of the script has been"
			    " unsuccessful :\n%s", CSTR(errmsg));
  str_free (errmsg);
}

void
EllissWindow::cleanScriptErrors ()
{
  scripttext->changeStyle(0, scripttext->getLength(), 0);
}

void
EllissWindow::setErrorRegion (int sl, int el)
{
  int cl = 1, ns = 0, next;
  FXString text(scripttext->getText());
  const char *base = text.text();

  while (cl < sl)
    {
      const char *ptr = strchr (base, '\n');
      if (! ptr)
	break;
      ptr ++;
      ns += ptr - base;
      base = ptr;
      cl ++;
    }

  if (cl < sl)
    return;

  next = 0;
  while (cl <= el)
    {
      const char *ptr = strchr (base, '\n');
      if (! ptr)
	break;
      ptr ++;
      next += ptr - base;
      base = ptr;
      cl ++;
    }
  
  scripttext->changeStyle(ns, next, 1);
}

float
timeval_subtract (struct timeval *x, struct timeval *y)
{
  float result = y->tv_sec - x->tv_sec;
  result += (y->tv_usec - x->tv_usec) / 1.0E6;
  return result;
}

int
process_foxgui_events (void *data, float progr, const char *msg)
{
  const float wait_time = 0.4, progress_thresold = 0.5;
  ProgressInfo *info = (ProgressInfo *) data;
  struct timeval current[1];

  if (gettimeofday (current, NULL) != 0)
    return 0;

  float diff = timeval_subtract (info->start, current);
  const int multfactor = 4096;

  if (info->dialog == NULL)
    {
      if (diff < wait_time || progr > progress_thresold)
	return 0;

      info->dialog = new FXProgressDialog(info->window,
					  "Fit is running",
					  "Please wait...",
					  PROGRESSDIALOG_CANCEL);

      info->dialog->setTotal(multfactor);
      info->dialog->setBarStyle(PROGRESSBAR_PERCENTAGE);
      info->dialog->create();
      info->dialog->show(PLACEMENT_SCREEN);

      info->dialog->setProgress((int) (progr * multfactor));
      info->dialog->repaint();

      info->app->beginWaitCursor();
      
      return 0;
    }
  
  if (info->dialog)
    {
      info->dialog->setProgress((int) (progr * multfactor));
      info->dialog->repaint();
      if (msg)
	info->dialog->setMessage(msg);

      info->app->runModalWhileEvents(info->dialog);

      return (info->dialog->shown() ? 0 : 1);
    }

  return 0;
}
