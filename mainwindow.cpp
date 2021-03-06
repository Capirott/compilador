#include <QtWidgets>
#include <iostream>
#include <sstream>
#include <map>
#include <regex>
#include <vector>

#define TAMANHOTABELA 1000 //Quantidade de variaveis suportadas pela linguagem
#define TAMANHOVARIAVEL 255
#define REGRA0 999
#define REGRA00 998


using std::cout;
using std::to_string;
using std::endl;

#include "mainwindow.h"

MainWindow::MainWindow()
    : textEdit(new QPlainTextEdit)

{
    setCentralWidget(textEdit);

    createActions();
    createStatusBar();

    readSettings();

    connect(textEdit->document(), &QTextDocument::contentsChanged,
            this, &MainWindow::documentWasModified);

#ifndef QT_NO_SESSIONMANAGER
    QGuiApplication::setFallbackSessionManagementEnabled(false);
    connect(qApp, &QGuiApplication::commitDataRequest,
            this, &MainWindow::commitData);
#endif

    setCurrentFile("QString()");
    loadFile(":/files/file.txt");
    setUnifiedTitleAndToolBarOnMac(true);
}



void MainWindow::closeEvent(QCloseEvent *event)

{
    if (maybeSave()) {
        writeSettings();
        event->accept();
    } else {
        event->ignore();
    }
}



void MainWindow::newFile()

{
    if (maybeSave()) {
        textEdit->clear();
        setCurrentFile(QString());
    }
}

void MainWindow::open()

{
    if (maybeSave()) {
        QString fileName = QFileDialog::getOpenFileName(this);
        if (!fileName.isEmpty())
            loadFile(fileName);
    }
}

bool MainWindow::save()

{
    if (curFile.isEmpty()) {
        return saveAs();
    } else {
        return saveFile(curFile);
    }
}

inline void addVariableToTable(std::string variable, std::map<std::string, int> symbols)
{
    std::string delim = "$";
    auto start = 0U;
    auto end = variable.find(delim);
    symbols.insert(std::pair<std::string, int>(variable.substr(start, end - start),0));
}

inline bool verifySemanticRule1(std::string variable)
{
    return std::regex_match(variable, std::regex("[a-zA-Z][0-9a-zA-Z]*\\$"));
}

inline bool verifyNumber(std::string str)
{
    foreach (char ch, str) {
        if (ch < '0' || ch > '9')
            return false;
    }
    return true;
}

inline bool verifyVariable(std::string &variable, std::map<std::string, int> &symbols)
{

    if (variable.back() == '$')
    {
        if (symbols[variable])
        {
            return true;
        }
        else
        {
            throw std::string("Variável não declarada");
        }
    }
    return false;
}

bool MainWindow::compile()
{
    using std::string;
    string output;
    string token;
    std::stringstream ss(textEdit->toPlainText().toStdString());
    std::map<string, int> symbols;
    int state = 0;
    try
    {
        while (!ss.eof())
        {
//            //~ clear the string to avoid empty lines
//            token.clear();
            ss >> token;
            //std::cout << state << endl;
            switch (state)
            {
            case 0:
                if (token == "programa")
                {
                    output += "#include <stdio.h>\n";
                    state = 1;
                } else {
                    throw string("Esperado Id=programa");
                }
                break;
            case 1:
                if (token == "var")
                {
                    output += "int ";
                    state = 2;
                } else {
                    throw string("Esperado Id=var");
                }
                break;
            case 2:
            case 3:
                if (token == ";")
                {
                    if (output.back() == ',')
                    {
                        output.pop_back();
                    }
                    output += ";\n\nint main()\n{\n";
                    state = 4;
                }
                else
                {
                    if (verifySemanticRule1(token))
                    {
                        if (token.back() == '$' && token.length() > TAMANHOVARIAVEL)
                        {
                            throw string("Semantic Error " + to_string(REGRA00) + ": Variavel " + token + " com nome muito grande. (MAX=" + to_string(TAMANHOVARIAVEL) + "\n");
                        }
                        if (symbols.size() < TAMANHOTABELA)
                        {
                            //~ Adiciona variavel na tabela
                            if(symbols[token])
                            {
                                throw string("Variavel já foi declarada");
                            }
                            symbols[token] = true;
                            token.pop_back();
                            output += token + ",";
                        }
                        else
                        {
                            throw string("Semantic Error " + to_string(REGRA0) + ": Tabela de Simbolos cheia. Numero de entradas=" + to_string(symbols.size()) + " variaveis\n");
                        }
                    }
                    else
                    {
                        if (token.back() == '$')
                        {
                            throw string("Variavel invalida");
                        }
                        else
                        {
                            throw string("Esperando Id=; ou uma variavel");
                        }
                    }
                }
                break;
            case 4:
                if (token == "leia")
                {
                    output += "\tscanf(\"%d\", &";
                    state = 5;
                }
                else
                {
                    throw string("Esperando Id=leia");
                }
                break;
            case 5:
                if (verifyVariable(token, symbols))
                {
                    token.pop_back();
                    output += token + ");\n";
                    state = 6;
                }
                else
                {
                    throw string("Esperando uma variavel");
                }
                break;
            case 6:
                if (token == "leia")
                {
                    output += "\tscanf(\"%d\", &";
                    state = 5;
                }
                else if (token == "escreva")
                {
                    output += "\tprintf(\"";
                    state = 7;
                }
                else
                {
                    throw string("Esperado Id=leia ou escreva");
                }
                break;
            case 7:
                if (token == "(")
                {
                    state = 9;
                }
                else if (verifyVariable(token, symbols))
                {
                    token.pop_back();
                    output += "%d\\n\"," + token + ");\n";
                    state = 8;
                }
                else
                {
                    cout << "id: " << token << endl;
                    throw string("Esperado Id=( ou uma variável");
                }
                break;
            case 8:
            case 10:
                if (token == "leia")
                {
                    output += "\tscanf(\"%d\", &";
                    state = 5;
                }
                else if (token == "escreva")
                {
                    output += "\tprintf(\"";
                    state = 7;
                }
                else if (token == "at")
                {
                    state = 11;
                }
                else
                {
                    throw string("Esperado Id=at ou leia ou escreva");
                }
                break;
            case 9:
                if (token == ")")
                {
                    output += "\\n\");\n";
                    state = 10;
                }
                else
                {
                    output += token + " ";
                }
                break;
            case 11:
                if (verifyVariable(token, symbols))
                {
                    token.pop_back();
                    output += "\t" + token;
                    state = 12;
                }
                else
                {
                    throw string("Esperando uma variavel");
                }
                break;
            case 12:
                if (token == "=")
                {
                    output += token;
                    state = 13;
                }
                else
                {
                    throw string("Esperando id= =");
                }
                break;
            case 13:
                if (verifyVariable(token, symbols))
                {
                    token.pop_back();
                    output += token + ";\n";
                    state = 15;
                }
                else if (verifyNumber(token))
                {
                    output += token + ";\n";
                    state = 14;
                }
                else
                {
                    throw string("Esperado um numero inteiro ou uma variável");
                }
                break;
            case 14:
            case 15:
                if (token == "at")
                {
                    state = 11;
                }
                else if (token == "se")
                {
                    output += "\tif (";
                    state = 16;
                }
                else
                {
                    throw string("Esperando id=se");
                }
                break;
            case 16:
                if (verifyVariable(token, symbols))
                {
                    token.pop_back();
                    output += token;
                    state = 17;
                }
                else
                {
                    throw string("Esperando uma variavel");
                }
                break;
            case 17:
                if (token == ">" || token == "<" || token == ">=" || token == "<=" || token == "!=")
                {
                    output += token;
                    state = 18;
                }
                else
                {
                    throw string("Esperado Operadores (<,>,<=,>=,==,!=)");
                }
                break;
            case 18:
                if (verifyVariable(token, symbols))
                {
                    token.pop_back();
                    output += token + ")\n";
                    state = 20;
                }
                else if (verifyNumber(token))
                {
                    output += token + ")\n";
                    state = 19;
                }
                else
                {
                    throw string("Esperado um numero inteiro ou uma variável");
                }
                break;
            case 19:
            case 20:
                if (token == "entao")
                {
                    output += "\t{\n";
                    state = 21;
                }
                else
                {
                    throw string("Esperando id=senao");
                }
                break;
            case 21:
                if (token == "at")
                {
                    state = 22;
                }
                else
                {
                    throw string("Esperando id=at");
                }
                break;
            case 22:
                if (verifyVariable(token, symbols))
                {
                    token.pop_back();
                    output += "\t\t" + token;
                    state = 23;
                }
                else
                {
                    throw string("Esperando uma variavel");
                }
                break;
            case 23:
                if (token == "=")
                {
                    output += token;
                    state = 24;
                }
                else
                {
                    throw string("Esperando id= =");
                }
                break;
            case 24:
            case 26:
                if (verifyVariable(token, symbols))
                {
                    token.pop_back();
                    output += token;
                    state = 25;
                }
                else if (verifyNumber(token))
                {
                    output += token;
                    state = 27;
                }
                else
                {
                    throw string("Esperado um numero inteiro ou uma variável");
                }
                break;
            case 25:
            case 27:
                if (token == "+" || token == "-" || token == "*")
                {
                    output += token;
                    state = 26;
                }
                else if (token == ";")
                {
                    output += token + "\n\t}\n";
                    state = 28;
                }
                else
                {
                    throw string("Esperado Operacoes (+,-,*) ou ;");
                }
                break;
            case 28:
                if (token == "senao")
                {
                    output += "\telse\n";
                    state = 29;
                }
                else
                {
                    throw string("Esperando id=senao");
                }
                break;
            case 29:
                if (token == "escreva")
                {
                    output += "\t\tprintf(\"";
                    state = 30;
                }
                else
                {
                    throw string("Esperando id=escreva");
                }
                break;
            case 30:
                if (token == "(")
                {
                    state = 31;
                }
                else
                {
                    throw string("Esperando id=(");
                }
                break;
            case 31:
                if (token == ")")
                {
                    output += " \\n\");\n";
                    state = 32;
                }
                else
                {
                    output += token;
                }
                break;
            case 32:
                if (token == "fim")
                {
                    output += "\texit(0);\n}";
                    state = 33;
                }
                else
                {
                    throw string("Esperando id=fim");
                }
                break;
            }
        }

        //system("C:\\Users\\erick\\Desktop\\qtenv2.bat");
        textEdit->setPlainText(tr(output.c_str()));
        //std::cout << "Compiled Program:\n" + output << endl;
        //QMessageBox::warning(this, tr("Compilation success!"),tr(output.c_str()), QMessageBox::Ok);
    }
    catch (string error)
    {
        QMessageBox::warning(this, tr("Compilation error!"),tr(error.c_str()), QMessageBox::Abort);
    }

    return false;
}

bool MainWindow::saveAs()

{
    QFileDialog dialog(this);
    dialog.setWindowModality(Qt::WindowModal);
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    if (dialog.exec() != QDialog::Accepted)
        return false;
    return saveFile(dialog.selectedFiles().first());
}

void MainWindow::about()

{
    QMessageBox::about(this, tr("About Application"),
                       tr("The <b>Application</b> example demonstrates how to "
                          "write modern GUI applications using Qt, with a menu bar, "
                          "toolbars, and a status bar."));
}



void MainWindow::documentWasModified()

{
    setWindowModified(textEdit->document()->isModified());
}

void MainWindow::createActions()

{

    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    QToolBar *fileToolBar = addToolBar(tr("File"));
    const QIcon newIcon = QIcon::fromTheme("document-new", QIcon(":/images/new.png"));
    QAction *newAct = new QAction(newIcon, tr("&New"), this);
    newAct->setShortcuts(QKeySequence::New);
    newAct->setStatusTip(tr("Create a new file"));
    connect(newAct, &QAction::triggered, this, &MainWindow::newFile);
    fileMenu->addAction(newAct);
    fileToolBar->addAction(newAct);


    const QIcon openIcon = QIcon::fromTheme("document-open", QIcon(":/images/open.png"));
    QAction *openAct = new QAction(openIcon, tr("&Open..."), this);
    openAct->setShortcuts(QKeySequence::Open);
    openAct->setStatusTip(tr("Open an existing file"));
    connect(openAct, &QAction::triggered, this, &MainWindow::open);
    fileMenu->addAction(openAct);
    fileToolBar->addAction(openAct);


    const QIcon saveIcon = QIcon::fromTheme("document-save", QIcon(":/images/save.png"));
    QAction *saveAct = new QAction(saveIcon, tr("&Save"), this);
    saveAct->setShortcuts(QKeySequence::Save);
    saveAct->setStatusTip(tr("Save the document to disk"));
    connect(saveAct, &QAction::triggered, this, &MainWindow::save);
    fileMenu->addAction(saveAct);
    fileToolBar->addAction(saveAct);

    const QIcon compileIcon = QIcon::fromTheme("document-refresh", QIcon(":/images/compile.png"));
    QAction *compileAct = new QAction(compileIcon, tr("&Compile"), this);
    compileAct->setStatusTip(tr("Compile the program"));
    compileAct->setShortcuts(QKeySequence::Refresh);
    connect(compileAct, &QAction::triggered, this, &MainWindow::compile);
    fileMenu->addAction(compileAct);
    fileToolBar->addAction(compileAct);

    const QIcon saveAsIcon = QIcon::fromTheme("document-save-as");
    QAction *saveAsAct = fileMenu->addAction(saveAsIcon, tr("Save &As..."), this, &MainWindow::saveAs);
    saveAsAct->setShortcuts(QKeySequence::SaveAs);
    saveAsAct->setStatusTip(tr("Save the document under a new name"));

    fileMenu->addSeparator();

    const QIcon exitIcon = QIcon::fromTheme("application-exit");
    QAction *exitAct = fileMenu->addAction(exitIcon, tr("E&xit"), this, &QWidget::close);
    exitAct->setShortcuts(QKeySequence::Quit);

    exitAct->setStatusTip(tr("Exit the application"));

    QMenu *editMenu = menuBar()->addMenu(tr("&Edit"));
    QToolBar *editToolBar = addToolBar(tr("Edit"));

#ifndef QT_NO_CLIPBOARD
    const QIcon cutIcon = QIcon::fromTheme("edit-cut", QIcon(":/images/cut.png"));
    QAction *cutAct = new QAction(cutIcon, tr("Cu&t"), this);

    cutAct->setShortcuts(QKeySequence::Cut);
    cutAct->setStatusTip(tr("Cut the current selection's contents to the "
                            "clipboard"));
    connect(cutAct, &QAction::triggered, textEdit, &QPlainTextEdit::cut);
    editMenu->addAction(cutAct);
    editToolBar->addAction(cutAct);

    const QIcon copyIcon = QIcon::fromTheme("edit-copy", QIcon(":/images/copy.png"));
    QAction *copyAct = new QAction(copyIcon, tr("&Copy"), this);
    copyAct->setShortcuts(QKeySequence::Copy);
    copyAct->setStatusTip(tr("Copy the current selection's contents to the "
                             "clipboard"));
    connect(copyAct, &QAction::triggered, textEdit, &QPlainTextEdit::copy);
    editMenu->addAction(copyAct);
    editToolBar->addAction(copyAct);

    const QIcon pasteIcon = QIcon::fromTheme("edit-paste", QIcon(":/images/paste.png"));
    QAction *pasteAct = new QAction(pasteIcon, tr("&Paste"), this);
    pasteAct->setShortcuts(QKeySequence::Paste);
    pasteAct->setStatusTip(tr("Paste the clipboard's contents into the current "
                              "selection"));
    connect(pasteAct, &QAction::triggered, textEdit, &QPlainTextEdit::paste);
    editMenu->addAction(pasteAct);
    editToolBar->addAction(pasteAct);

    menuBar()->addSeparator();

#endif // !QT_NO_CLIPBOARD

    QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));
    QAction *aboutAct = helpMenu->addAction(tr("&About"), this, &MainWindow::about);
    aboutAct->setStatusTip(tr("Show the application's About box"));
    QAction *aboutQtAct = helpMenu->addAction(tr("About &Qt"), qApp, &QApplication::aboutQt);
    aboutQtAct->setStatusTip(tr("Show the Qt library's About box"));

#ifndef QT_NO_CLIPBOARD
    cutAct->setEnabled(false);

    copyAct->setEnabled(false);
    connect(textEdit, &QPlainTextEdit::copyAvailable, cutAct, &QAction::setEnabled);
    connect(textEdit, &QPlainTextEdit::copyAvailable, copyAct, &QAction::setEnabled);
#endif // !QT_NO_CLIPBOARD
}

void MainWindow::createStatusBar()

{
    statusBar()->showMessage(tr("Ready"));
}

void MainWindow::readSettings()

{
    QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());
    const QByteArray geometry = settings.value("geometry", QByteArray()).toByteArray();
    if (geometry.isEmpty()) {
        const QRect availableGeometry = QApplication::desktop()->availableGeometry(this);
        resize(availableGeometry.width() / 3, availableGeometry.height() / 2);
        move((availableGeometry.width() - width()) / 2,
             (availableGeometry.height() - height()) / 2);
    } else {
        restoreGeometry(geometry);
    }
}

void MainWindow::writeSettings()

{
    QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());
    settings.setValue("geometry", saveGeometry());
}

bool MainWindow::maybeSave()

{
    if (!textEdit->document()->isModified())
        return true;
    const QMessageBox::StandardButton ret
            = QMessageBox::warning(this, tr("Application"),
                                   tr("The document has been modified.\n"
                                      "Do you want to save your changes?"),
                                   QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    switch (ret) {
    case QMessageBox::Save:
        return save();
    case QMessageBox::Cancel:
        return false;
    default:
        break;
    }
    return true;
}

void MainWindow::loadFile(const QString &fileName)

{
    QFile file(fileName);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("Application"),
                             tr("Cannot read file %1:\n%2.")
                             .arg(QDir::toNativeSeparators(fileName), file.errorString()));
        return;
    }

    QTextStream in(&file);
#ifndef QT_NO_CURSOR
    QApplication::setOverrideCursor(Qt::WaitCursor);
#endif
    textEdit->setPlainText(in.readAll());
#ifndef QT_NO_CURSOR
    QApplication::restoreOverrideCursor();
#endif

    setCurrentFile(fileName);
    statusBar()->showMessage(tr("File loaded"), 2000);
}



bool MainWindow::saveFile(const QString &fileName)

{
    QFile file(fileName);
    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("Application"),
                             tr("Cannot write file %1:\n%2.")
                             .arg(QDir::toNativeSeparators(fileName),
                                  file.errorString()));
        return false;
    }

    QTextStream out(&file);
#ifndef QT_NO_CURSOR
    QApplication::setOverrideCursor(Qt::WaitCursor);
#endif
    out << textEdit->toPlainText();
#ifndef QT_NO_CURSOR
    QApplication::restoreOverrideCursor();
#endif

    setCurrentFile(fileName);
    statusBar()->showMessage(tr("File saved"), 2000);
    return true;
}

void MainWindow::setCurrentFile(const QString &fileName)

{
    curFile = fileName;
    textEdit->document()->setModified(false);
    setWindowModified(false);

    QString shownName = curFile;
    if (curFile.isEmpty())
        shownName = "untitled.txt";
    setWindowFilePath(shownName);
}

QString MainWindow::strippedName(const QString &fullFileName)

{
    return QFileInfo(fullFileName).fileName();
}

#ifndef QT_NO_SESSIONMANAGER
void MainWindow::commitData(QSessionManager &manager)
{
    if (manager.allowsInteraction()) {
        if (!maybeSave())
            manager.cancel();
    } else {
        // Non-interactive: save without asking
        if (textEdit->document()->isModified())
            save();
    }
}
#endif
