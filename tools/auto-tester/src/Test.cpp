//
//  Test.cpp
//
//  Created by Nissim Hadar on 2 Nov 2017.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "Test.h"

#include <assert.h>
#include <QtCore/QTextStream>
#include <QDirIterator>

#include <quazip5/quazip.h>
#include <quazip5/JlCompress.h>

#include "ui/AutoTester.h"
extern AutoTester* autoTester;

Test::Test() {
    QString regex(EXPECTED_IMAGE_PREFIX + QString("\\\\d").repeated(NUM_DIGITS) + IMAGE_FORMAT);
    
    expectedImageFilenameFormat = QRegularExpression(regex);

    mismatchWindow.setModal(true);
}

bool Test::createTestResultsFolderPathIfNeeded(QString directory) {
    // The test results folder is located in the root of the tests (i.e. for recursive test evaluation)
    if (testResultsFolderPath == "") {
        testResultsFolderPath =  directory + "/" + TEST_RESULTS_FOLDER;
        QDir testResultsFolder(testResultsFolderPath);

        if (testResultsFolder.exists()) {
            testResultsFolder.removeRecursively();
        }

        // Create a new test results folder
        return QDir().mkdir(testResultsFolderPath);
    } else {
        return true;
    }
}

void Test::zipAndDeleteTestResultsFolder() {
    QString zippedResultsFileName { testResultsFolderPath + ".zip" };
    QFileInfo fileInfo(zippedResultsFileName);
    if (!fileInfo.exists()) {
        QFile::remove(zippedResultsFileName);
    }

    QDir testResultsFolder(testResultsFolderPath);
    if (!testResultsFolder.isEmpty()) {
        JlCompress::compressDir(testResultsFolderPath + ".zip", testResultsFolderPath);
    }

    testResultsFolder.removeRecursively();

    //In all cases, for the next evaluation
    testResultsFolderPath = "";
    index = 1;
}

bool Test::compareImageLists(bool isInteractiveMode, QProgressBar* progressBar) {
    progressBar->setMinimum(0);
    progressBar->setMaximum(expectedImagesFullFilenames.length() - 1);
    progressBar->setValue(0);
    progressBar->setVisible(true);

    // Loop over both lists and compare each pair of images
    // Quit loop if user has aborted due to a failed test.
    const double THRESHOLD { 0.999 };
    bool success{ true };
    bool keepOn{ true };
    for (int i = 0; keepOn && i < expectedImagesFullFilenames.length(); ++i) {
        // First check that images are the same size
        QImage resultImage(resultImagesFullFilenames[i]);
        QImage expectedImage(expectedImagesFullFilenames[i]);

        if (resultImage.width() != expectedImage.width() || resultImage.height() != expectedImage.height()) {
            messageBox.critical(0, "Internal error", "Images are not the same size");
            exit(-1);
        }

        double similarityIndex;  // in [-1.0 .. 1.0], where 1.0 means images are identical
        try {
            similarityIndex = imageComparer.compareImages(resultImage, expectedImage);
        } catch (...) {
            messageBox.critical(0, "Internal error", "Image not in expected format");
            exit(-1);
        }

        if (similarityIndex < THRESHOLD) {
            TestFailure testFailure = TestFailure{
                (float)similarityIndex,
                expectedImagesFullFilenames[i].left(expectedImagesFullFilenames[i].lastIndexOf("/") + 1), // path to the test (including trailing /)
                QFileInfo(expectedImagesFullFilenames[i].toStdString().c_str()).fileName(),  // filename of expected image
                QFileInfo(resultImagesFullFilenames[i].toStdString().c_str()).fileName()     // filename of result image
            };

            mismatchWindow.setTestFailure(testFailure);

            if (!isInteractiveMode) {
                appendTestResultsToFile(testResultsFolderPath, testFailure, mismatchWindow.getComparisonImage());
                success = false;
            } else {
                mismatchWindow.exec();

                switch (mismatchWindow.getUserResponse()) {
                    case USER_RESPONSE_PASS:
                        break;
                    case USE_RESPONSE_FAIL:
                        appendTestResultsToFile(testResultsFolderPath, testFailure, mismatchWindow.getComparisonImage());
                        success = false;
                        break;
                    case USER_RESPONSE_ABORT:
                        keepOn = false;
                        success = false;
                        break;
                    default:
                        assert(false);
                        break;
                }
            }
        }

        progressBar->setValue(i);
    }

    progressBar->setVisible(false);
    return success;
}

void Test::appendTestResultsToFile(QString testResultsFolderPath, TestFailure testFailure, QPixmap comparisonImage) {
    if (!QDir().exists(testResultsFolderPath)) {
        messageBox.critical(0, "Internal error", "Folder " + testResultsFolderPath + " not found");
        exit(-1);
    }

    QString failureFolderPath { testResultsFolderPath + "/" + "Failure_" + QString::number(index) };
    if (!QDir().mkdir(failureFolderPath)) {
        messageBox.critical(0, "Internal error", "Failed to create folder " + failureFolderPath);
        exit(-1);
    }
    ++index;

    QFile descriptionFile(failureFolderPath + "/" + TEST_RESULTS_FILENAME);
    if (!descriptionFile.open(QIODevice::ReadWrite)) {
        messageBox.critical(0, "Internal error", "Failed to create file " + TEST_RESULTS_FILENAME);
        exit(-1);
    }

    // Create text file describing the failure
    QTextStream stream(&descriptionFile);
    stream << "Test failed in folder " << testFailure._pathname.left(testFailure._pathname.length() - 1) << endl; // remove trailing '/'
    stream << "Expected image was    " << testFailure._expectedImageFilename << endl;
    stream << "Actual image was      " << testFailure._actualImageFilename << endl;
    stream << "Similarity index was  " << testFailure._error << endl;

    descriptionFile.close();

    // Copy expected and actual images, and save the difference image
    QString sourceFile;
    QString destinationFile;

    sourceFile = testFailure._pathname + testFailure._expectedImageFilename;
    destinationFile = failureFolderPath + "/" + "Expected Image.jpg";
    if (!QFile::copy(sourceFile, destinationFile)) {
        messageBox.critical(0, "Internal error", "Failed to copy " + sourceFile + " to " + destinationFile);
        exit(-1);
    }

    sourceFile = testFailure._pathname + testFailure._actualImageFilename;
    destinationFile = failureFolderPath + "/" + "Actual Image.jpg";
    if (!QFile::copy(sourceFile, destinationFile)) {
        messageBox.critical(0, "Internal error", "Failed to copy " + sourceFile + " to " + destinationFile);
        exit(-1);
    }

    comparisonImage.save(failureFolderPath + "/" + "Difference Image.jpg");
}

void Test::startTestsEvaluation() {
    // Get list of JPEG images in folder, sorted by name
    pathToTestResultsDirectory = QFileDialog::getExistingDirectory(nullptr, "Please select folder containing the test images", ".", QFileDialog::ShowDirsOnly);
    if (pathToTestResultsDirectory == "") {
        return;
    }

    // Quit if test results folder could not be created
    if (!createTestResultsFolderPathIfNeeded(pathToTestResultsDirectory)) {
        return;
    }

    // Create two lists.  The first is the test results,  the second is the expected images
    // The expected images are represented as a URL to enabel download from GitHub
    // Images that are in the wrong format are ignored.

    QStringList sortedTestResultsFilenames = createListOfAll_IMAGE_FORMAT_imagesInDirectory(pathToTestResultsDirectory);
    QStringList expectedImagesURLs;

    const QString URLPrefix("https://raw.githubusercontent.com");
    const QString githubUser("NissimHadar");
    const QString testsRepo("hifi_tests");
    const QString branch("addRecursionToAutotester");

    resultImagesFullFilenames.clear();
    expectedImagesFilenames.clear();
    expectedImagesFullFilenames.clear();

    foreach(QString currentFilename, sortedTestResultsFilenames) {
        QString fullCurrentFilename = pathToTestResultsDirectory + "/" + currentFilename;
        if (isInSnapshotFilenameFormat(currentFilename)) {
            resultImagesFullFilenames << fullCurrentFilename;

            QString expectedImagePartialSourceDirectory = getExpectedImagePartialSourceDirectory(currentFilename);

            // Images are stored on GitHub as ExpectedImage_ddddd.png
            // Extract the digits at the end of the filename (exluding the file extension)
            QString expectedImageFilenameTail = currentFilename.left(currentFilename.length() - 4).right(NUM_DIGITS);
            QString expectedImageStoredFilename = EXPECTED_IMAGE_PREFIX + expectedImageFilenameTail + IMAGE_FORMAT;

            QString imageURLString(URLPrefix + "/" + githubUser + "/" + testsRepo + "/" + branch + "/" + 
                expectedImagePartialSourceDirectory + "/" + expectedImageStoredFilename);

            expectedImagesURLs << imageURLString;

            // The image retrieved from Github needs a unique name
            QString expectedImageFilename = currentFilename.replace("/", "_").replace(".", "_EI.");

            expectedImagesFilenames << expectedImageFilename;
            expectedImagesFullFilenames << pathToTestResultsDirectory + "/" + expectedImageFilename;
        }
    }

    autoTester->downloadImages(expectedImagesURLs, pathToTestResultsDirectory, expectedImagesFilenames);
}

void Test::finishTestsEvaluation(bool interactiveMode, QProgressBar* progressBar) {
    bool success = compareImageLists(interactiveMode, progressBar);
    
    if (success) {
        messageBox.information(0, "Success", "All images are as expected");
    } else {
        messageBox.information(0, "Failure", "One or more images are not as expected");
    }

    zipAndDeleteTestResultsFolder();
}

bool Test::isAValidDirectory(QString pathname) {
    // Only process directories
    QDir dir(pathname);
    if (!dir.exists()) {
        return false;
    }

    // Ignore '.', '..' directories
    if (pathname[pathname.length() - 1] == '.') {
        return false;
    }

    return true;
}

void Test::importTest(QTextStream& textStream, const QString& testPathname) {
    textStream << "Script.include(\"" << "file:///" << testPathname + "?raw=true\");" << endl;
}

// Creates a single script in a user-selected folder.
// This script will run all text.js scripts in every applicable sub-folder
void Test::createRecursiveScript() {
    // Select folder to start recursing from
    QString topLevelDirectory = QFileDialog::getExistingDirectory(nullptr, "Please select folder that will contain the top level test script", ".", QFileDialog::ShowDirsOnly);
    if (topLevelDirectory == "") {
        return;
    }

    const QString recursiveTestsFilename("testRecursive.js");
    QFile allTestsFilename(topLevelDirectory + "/" + recursiveTestsFilename);
    if (!allTestsFilename.open(QIODevice::WriteOnly | QIODevice::Text)) {
        messageBox.critical(0,
            "Internal Error",
            "Failed to create \"" + recursiveTestsFilename + "\" in directory \"" + topLevelDirectory + "\""
        );

        exit(-1);
    }

    QTextStream textStream(&allTestsFilename);
    textStream << "// This is an automatically generated file, created by auto-tester" << endl << endl;

    textStream << "var autoTester = Script.require(\"https://github.com/highfidelity/hifi_tests/blob/master/tests/utils/autoTester.js?raw=true\");" << endl;
    textStream << "autoTester.enableRecursive();" << endl << endl;

    QVector<QString> testPathnames;

    // First test if top-level folder has a test.js file
    const QString testPathname{ topLevelDirectory + "/" + TEST_FILENAME };
    QFileInfo fileInfo(testPathname);
    if (fileInfo.exists()) {
        // Current folder contains a test
        importTest(textStream, testPathname);

        testPathnames << testPathname;
    }

    QDirIterator it(topLevelDirectory.toStdString().c_str(), QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString directory = it.next();

        // Only process directories
        QDir dir(directory);
        if (!isAValidDirectory(directory)) {
            continue;
        }

        const QString testPathname{ directory + "/" + TEST_FILENAME };
        QFileInfo fileInfo(testPathname);
        if (fileInfo.exists()) {
            // Current folder contains a test
            importTest(textStream, testPathname);

            testPathnames << testPathname;
        }
    }

    if (testPathnames.length() <= 0) {
        messageBox.information(0, "Failure", "No \"" + TEST_FILENAME + "\" files found");
        allTestsFilename.close();
        return;
    }

    textStream << endl;
    textStream << "autoTester.runRecursive();" << endl;

    allTestsFilename.close();
    messageBox.information(0, "Success", "Script has been created");
}

void Test::createRecursiveScriptsRecursively() {
}

void Test::createTest() {
    // Rename files sequentially, as ExpectedResult_00000.jpeg, ExpectedResult_00001.jpg and so on
    // Any existing expected result images will be deleted
    QString imageSourceDirectory = QFileDialog::getExistingDirectory(nullptr, "Please select folder containing the test images", ".", QFileDialog::ShowDirsOnly);
    if (imageSourceDirectory == "") {
        return;
    }

    QStringList sortedImageFilenames = createListOfAll_IMAGE_FORMAT_imagesInDirectory(imageSourceDirectory);

    int i = 1; 
    const int maxImages = pow(10, NUM_DIGITS);
    foreach (QString currentFilename, sortedImageFilenames) {
        QString fullCurrentFilename = imageSourceDirectory + "/" + currentFilename;
        if (isInSnapshotFilenameFormat(currentFilename)) {
            if (i >= maxImages) {
                messageBox.critical(0, "Error", "More than " + QString::number(maxImages) + " images not supported");
                exit(-1);
            }
            QString newFilename = "ExpectedImage_" + QString::number(i - 1).rightJustified(5, '0') + IMAGE_FORMAT;
            QString imageDestinationDirectory = getExpectedImageDestinationDirectory(currentFilename);
            QString fullNewFileName = imageDestinationDirectory + "/" + newFilename;

            try {
                QFile::remove(fullNewFileName);
                QFile::copy(fullCurrentFilename, fullNewFileName);
            } catch (...) {
                messageBox.critical(0, "Error", "Could not delete existing file: " + currentFilename + "\nTest creation aborted");
                exit(-1);
            }
            ++i;
        }
    }

    messageBox.information(0, "Success", "Test images have been created");
}

void Test::deleteOldSnapshots() {
    // Select folder to start recursing from
    QString topLevelDirectory = QFileDialog::getExistingDirectory(nullptr, "Please select root folder for snapshot deletion", ".", QFileDialog::ShowDirsOnly);
    if (topLevelDirectory == "") {
        return;
    }

    // Recurse over folders
    QDirIterator it(topLevelDirectory.toStdString().c_str(), QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString directory = it.next();

        // Only process directories
        QDir dir(directory);
        if (!isAValidDirectory(directory)) {
            continue;
        }

        QStringList sortedImageFilenames = createListOfAll_IMAGE_FORMAT_imagesInDirectory(directory);

        // Delete any file that is a snapshot (NOT the Expected Images)
        QStringList expectedImages;
        QStringList resultImages;
        foreach(QString currentFilename, sortedImageFilenames) {
            QString fullCurrentFilename = directory + "/" + currentFilename;
            if (isInSnapshotFilenameFormat(currentFilename)) {
                if (!QFile::remove(fullCurrentFilename)) {
                    messageBox.critical(0, "Error", "Could not delete existing file: " + currentFilename + "\nSnapshot deletion aborted");
                    exit(-1);
                }
            }
        }
    }
}

QStringList Test::createListOfAll_IMAGE_FORMAT_imagesInDirectory(QString pathToImageDirectory) {
    imageDirectory = QDir(pathToImageDirectory);
    QStringList nameFilters;
    nameFilters << "*" + IMAGE_FORMAT;

    return imageDirectory.entryList(nameFilters, QDir::Files, QDir::Name);
}

// Snapshots are files in the following format:
//      Filename contains no periods (excluding period before exception
//      Filename (i.e. without extension) contains _tests_ (this is based on all test scripts being within the tests folder
//      Last 5 characters in filename are digits
//      Extension is jpg
bool Test::isInSnapshotFilenameFormat(QString filename) {
    QStringList filenameParts = filename.split(".");

    bool filnameHasNoPeriods = (filenameParts.size() == 2);
    bool contains_tests = filenameParts[0].contains("_tests_");

    bool last5CharactersAreDigits;
    filenameParts[0].right(5).toInt(&last5CharactersAreDigits, 10);

    bool extensionIsIMAGE_FORMAT = filenameParts[1] == IMAGE_FORMAT.right(3); // without the period

    return (filnameHasNoPeriods && contains_tests && last5CharactersAreDigits && extensionIsIMAGE_FORMAT);
}

// For a file named "D_GitHub_hifi-tests_tests_content_entity_zone_create_0.jpg", the test directory is
// D:/GitHub/hifi-tests/tests/content/entity/zone/create
// This method assumes the filename is in the correct format
QString Test::getExpectedImageDestinationDirectory(QString filename) {
    QString filenameWithoutExtension = filename.split(".")[0];
    QStringList filenameParts = filenameWithoutExtension.split("_");

    QString result = filenameParts[0] + ":";

    for (int i = 1; i < filenameParts.length() - 1; ++i) {
        result += "/" + filenameParts[i];
    }

    return result;
}

// For a file named "D_GitHub_hifi-tests_tests_content_entity_zone_create_0.jpg", the source directory on GitHub
// is ...tests/content/entity/zone/create
// This is used to create the full URL
// This method assumes the filename is in the correct format
QString Test::getExpectedImagePartialSourceDirectory(QString filename) {
    QString filenameWithoutExtension = filename.split(".")[0];
    QStringList filenameParts = filenameWithoutExtension.split("_");

    int i { 0 };
    while (filenameParts[i] != "tests") {
        ++i;
    }

    QString result = filenameParts[i];

    for (int j = i + 1; j < filenameParts.length() - 1; ++j) {
        result += "/" + filenameParts[j];
    }

    return result;
}
