
Option Explicit


Dim wordApp 
Dim fso 
Dim rootPath 'As String
Dim myFile 'As String
Dim folder 'As Scripting.folder
Dim args

Set args = Wscript.Arguments
rootPath = args(0)


Set wordApp = CreateObject("Word.Application")
Set fso = CreateObject("Scripting.FileSystemObject")
wordApp.Visible = True
	
Set folder = fso.GetFolder(rootPath)

ProcessFolder folder

wordApp.Quit
Wscript.Echo "done"

    

Sub ProcessFolder(folder)
    Dim suffix 'As String
    Dim prefix 'As String
   
    'files
    Dim files 'As Scripting.files
    Dim file 'As Scripting.file
    
    Set files = folder.files
    For Each file In files
        'Wscript.Echo file.Name
        
        If (StrComp(Left(file.Name, 1), "~") <> 0) And Len(file.Name) > 5 Then
            suffix = Right(file.Name, 5)
            
            If StrComp(suffix, ".docx", vbTextCompare) = 0 Then
                prefix = Left(file.Name, Len(file.Name) - 5)

                Dim oldFilePath 'As String
                Dim newFilePath 'As String
    
                oldFilePath = folder.Path + "\" + file.Name
                newFilePath = folder.Path + "\" + prefix + ".doc"
                Wscript.Echo oldFilePath
                'Wscript.Echo  newFilePath
                
                Dim doc 'As Word.Document
                
                On Error Resume Next
                Set doc = wordApp.documents.Open(oldFilePath)
                
				If Err.Number <> 0 Then
					On Error GoTo 0
					Wscript.Echo "Error: " + oldFilePath
                Else
                    doc.SaveAs2 newFilePath, 0 ' wdFormatDocument
					On Error Goto 0

                    doc.Close 0 'wdDoNotSaveChanges

					On Error Resume Next
					file.Delete True
					On Error GoTo 0
                End If
                
            End If
            
        End If
        
    Next

    'folders
    Dim subFolder 'As Scripting.folder
    For Each subFolder In folder.SubFolders
        Wscript.Echo subFolder.Path
        ProcessFolder subFolder
        
        If Len(subFolder.Name) > 2 Then
            suffix = Right(subFolder.Name, 2)
            If StrComp(suffix, " A") = 0 Or _
                StrComp(suffix, " C") = 0 Or _
                StrComp(suffix, " E") = 0 Or _
                StrComp(suffix, " F") = 0 Or _
                StrComp(suffix, " R") = 0 Or _
                StrComp(suffix, " S") = 0 Then
                            
                prefix = Left(subFolder.Name, Len(subFolder.Name) - 2)
                subFolder.Name = prefix
            End If
        End If
    Next
    
End Sub

