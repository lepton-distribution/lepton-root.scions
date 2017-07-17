Imports System.IO
Imports System.Threading
Imports System.IO.MemoryMappedFiles
Imports System.IO.File
Imports System.IO.Pipes
Imports System.Math


Public Class LcdMatrixControl

    '
    Public DEFINE_COLOR_DOT_PIXEL_BPP = 4
    '
    Public DEFINE_COLOR_DOT_PIXEL_ON As Color = Color.FromArgb(&HFF, &HFF, &HCC, &H0)
    Public DEFINE_COLOR_DOT_PIXEL_OFF As Color = Color.Black

    '
    Public DEFINE_LCD_MATRIX_SIZE_IN_PIXEL_X As Integer = 256
    Public DEFINE_LCD_MATRIX_SIZE_IN_PIXEL_Y As Integer = 64

    '
    Private m_dot_size_x_inch As Single  'in pixel mm to inch
    Private m_dot_size_y_inch As Single  'in pixel mm to inch
    '
    Private m_dot_size_x_in_pixel As Integer 'in pixel unit
    Private m_dot_size_y_in_pixel As Integer  'in pixel unit

    Private m_display_dot_number_x As Integer
    Private m_display_dot_number_y As Integer

    Private m_display_size_x_in_pixel As Integer
    Private m_display_size_y_in_pixel As Integer

    '
    Private m_dotPenOff As New Pen(DEFINE_COLOR_DOT_PIXEL_OFF)
    Private m_dotBrushOff As New SolidBrush(DEFINE_COLOR_DOT_PIXEL_OFF)
    '
    Private m_dotPenOn As New Pen(DEFINE_COLOR_DOT_PIXEL_ON)
    Private m_dotBrushOn As New SolidBrush(DEFINE_COLOR_DOT_PIXEL_ON)

    '
    Private m_pictureBoxMatrixDisplay As New PictureBox()

    '
    Private m_bitmapMatrixDisplay As Bitmap
    Private m_graphicMatrixDisplay As Graphics

    '
    Private m_refresh_matrix_semaphore As Semaphore
    Private m_threadRefreshMatrixDisplay As Thread
    '
    Private m_frameBuffer(DEFINE_LCD_MATRIX_SIZE_IN_PIXEL_X * DEFINE_LCD_MATRIX_SIZE_IN_PIXEL_Y) As Byte

    Public Structure FrameBufferStructure
        Public data As Byte
    End Structure

    Public Sub RefreshMatrix()
        '
        Dim dot_x As Integer = 0
        Dim dot_y As Integer = 0

        '
        For i As Integer = 0 To m_frameBuffer.GetUpperBound(0)

            Select Case (DEFINE_COLOR_DOT_PIXEL_BPP)
                Case 4
                    Dim dot_bpp_value_lsb As Byte = m_frameBuffer(i) And &HF0
                    Dim dot_bpp_value_msb As Byte = m_frameBuffer(i) And &HF

                    If (dot_bpp_value_lsb = 0) Then
                        'pixel off
                        m_graphicMatrixDisplay.FillRectangle(m_dotBrushOff, dot_x, dot_y, m_dot_size_x_in_pixel, m_dot_size_y_in_pixel)
                    Else
                        'pixel on
                        m_graphicMatrixDisplay.FillRectangle(m_dotBrushOn, dot_x, dot_y, m_dot_size_x_in_pixel, m_dot_size_y_in_pixel)
                    End If
                    '
                    If (dot_x + m_dot_size_x_in_pixel < m_display_size_x_in_pixel) Then
                        dot_x = dot_x + m_dot_size_x_in_pixel
                    Else
                        dot_x = 0
                        dot_y = dot_y + m_dot_size_y_in_pixel
                    End If

                    '
                    If (dot_bpp_value_msb = 0) Then
                        'pixel off
                        m_graphicMatrixDisplay.FillRectangle(m_dotBrushOff, dot_x, dot_y, m_dot_size_x_in_pixel, m_dot_size_y_in_pixel)
                    Else
                        'pixel on
                        m_graphicMatrixDisplay.FillRectangle(m_dotBrushOn, dot_x, dot_y, m_dot_size_x_in_pixel, m_dot_size_y_in_pixel)
                    End If
                    '
                    If (dot_x + m_dot_size_x_in_pixel < m_display_size_x_in_pixel) Then
                        dot_x = dot_x + m_dot_size_x_in_pixel
                    Else
                        dot_x = 0
                        dot_y = dot_y + m_dot_size_y_in_pixel
                    End If

            End Select



        Next
        '
        m_pictureBoxMatrixDisplay.Image = m_bitmapMatrixDisplay

    End Sub

    Public Sub CreateBitmap()
        Dim myGraphics As Graphics = Me.CreateGraphics()

        '
        Dim screen_x_ppi As Single = myGraphics.DpiX
        Dim screen_y_ppi As Single = myGraphics.DpiY

        '
        m_dot_size_x_inch = 0.3 / 25.4 'dot size in mm to inch
        m_dot_size_y_inch = 0.3 / 25.4 'dot size in mm to inch

        '
        m_dot_size_x_in_pixel = m_dot_size_x_inch * screen_x_ppi 'in pixel unit
        m_dot_size_y_in_pixel = m_dot_size_y_inch * screen_y_ppi 'in pixel unit

        '
        m_display_dot_number_x = DEFINE_LCD_MATRIX_SIZE_IN_PIXEL_X
        m_display_dot_number_y = DEFINE_LCD_MATRIX_SIZE_IN_PIXEL_Y

        m_display_size_x_in_pixel = m_display_dot_number_x * m_dot_size_x_in_pixel
        m_display_size_y_in_pixel = m_display_dot_number_y * m_dot_size_y_in_pixel

        '
        m_dotPenOff = New Pen(DEFINE_COLOR_DOT_PIXEL_OFF)
        m_dotBrushOff = New SolidBrush(DEFINE_COLOR_DOT_PIXEL_OFF)
        '
        m_dotPenOn = New Pen(DEFINE_COLOR_DOT_PIXEL_ON)
        m_dotBrushOn = New SolidBrush(DEFINE_COLOR_DOT_PIXEL_ON)

        'pixel rendering
        Dim bitmap_pixel As New Bitmap(m_dot_size_x_in_pixel, m_dot_size_y_in_pixel)

        'matrix display
        m_pictureBoxMatrixDisplay.Size = New Size(m_display_size_x_in_pixel + 1, m_display_size_y_in_pixel + 1)
        Me.Controls.Add(m_pictureBoxMatrixDisplay)
        '
        m_bitmapMatrixDisplay = New Bitmap(m_display_size_x_in_pixel, m_display_size_y_in_pixel)
        m_graphicMatrixDisplay = Graphics.FromImage(m_bitmapMatrixDisplay)
        '
        Me.ClientSize = New Size(m_pictureBoxMatrixDisplay.Size.Width + 1, m_pictureBoxMatrixDisplay.Size.Height + 1)

        '
        m_graphicMatrixDisplay.FillRectangle(m_dotBrushOff, -1, -1, m_bitmapMatrixDisplay.Width + 1, m_bitmapMatrixDisplay.Height + 1)

        m_pictureBoxMatrixDisplay.Image = m_bitmapMatrixDisplay

        RefreshMatrix()

    End Sub

    Delegate Sub RefreshMatrixCallback()
    Private Sub RefreshMatrixSync()
        ' InvokeRequired required compares the thread ID of the
        ' calling thread to the thread ID of the creating thread.
        ' If these threads are different, it returns true.
        If Me.InvokeRequired Then
            Dim d As New RefreshMatrixCallback(AddressOf RefreshMatrix)
            Me.Invoke(d)
        Else
            RefreshMatrix()
        End If
    End Sub

    Private Sub ThreadProcRefreshMatrixSync()
        Dim offset As Integer = 0
        Dim length As Integer = DEFINE_LCD_MATRIX_SIZE_IN_PIXEL_X * DEFINE_LCD_MATRIX_SIZE_IN_PIXEL_Y
        Dim structureFrameBuffer As New FrameBufferStructure
        '
        Using mmf = System.IO.MemoryMappedFiles.MemoryMappedFile.CreateNew("MyFileMappingObject", length)

            While True

                m_refresh_matrix_semaphore.WaitOne()

                '
                Using accessor = mmf.CreateViewAccessor(offset, length)
                    accessor.ReadArray(0, m_frameBuffer, 0, length)
                End Using

                '
                RefreshMatrixSync()

            End While
        End Using


    End Sub
    '
    Private Sub CreateSharedMemory()
        Dim semaphoreCreated As Boolean
        '
        m_refresh_matrix_semaphore = New Semaphore(0, 1, "testmapsemaphore", semaphoreCreated)
        '
        m_threadRefreshMatrixDisplay = New Thread(New ThreadStart(AddressOf Me.ThreadProcRefreshMatrixSync))
        '
        m_threadRefreshMatrixDisplay.Start()
        '
    End Sub
    '
    Private Sub LcdMatrixControl_Load(sender As Object, e As EventArgs) Handles Me.Load
        CreateBitmap()
        '
        If (Not Me.DesignMode()) Then
            CreateSharedMemory()
        End If
    End Sub

    Private Sub LcdMatrixControl_Paint(sender As Object, e As PaintEventArgs) Handles Me.Paint
        RefreshMatrix()
        'MyBase.OnPaint(e)
    End Sub
End Class
