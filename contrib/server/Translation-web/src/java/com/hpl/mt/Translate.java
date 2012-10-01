package com.hpl.mt;

/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

import java.io.IOException;
import java.io.PrintWriter;
import java.net.URL;
import java.util.HashMap;
import java.util.logging.Level;
import java.util.logging.Logger;
import javax.servlet.ServletException;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import org.apache.xmlrpc.XmlRpcException;
import org.apache.xmlrpc.client.XmlRpcClient;
import org.apache.xmlrpc.client.XmlRpcClientConfigImpl;

/**
 *
 * @author ulanov
 */
public class Translate extends HttpServlet {

    /**
     * Processes requests for both HTTP
     * <code>GET</code> and
     * <code>POST</code> methods.
     *
     * @param request servlet request
     * @param response servlet response
     * @throws ServletException if a servlet-specific error occurs
     * @throws IOException if an I/O error occurs
     */
    protected void processRequest(HttpServletRequest request, HttpServletResponse response)
            throws ServletException, IOException {
        response.setContentType("text/html;charset=UTF-8");
        System.out.println("before" + request.getCharacterEncoding());
        request.setCharacterEncoding("UTF-8");
        System.out.println("after" + request.getCharacterEncoding());
        PrintWriter out = response.getWriter();
        try {
            /*
             * TODO output your page here. You may use following sample code.
             */
                // Create an instance of XmlRpcClient
                String textToTranslate = request.getParameter("text");
                XmlRpcClientConfigImpl config = new XmlRpcClientConfigImpl();
                config.setServerURL(new URL("http://localhost:9008/RPC2"));
                XmlRpcClient client = new XmlRpcClient();
                client.setConfig(config);
                // The XML-RPC data type used by mosesserver is <struct>. In Java, this data type can be represented using HashMap.
                HashMap<String,String> mosesParams = new HashMap<String,String>();
                mosesParams.put("text", textToTranslate);
                mosesParams.put("align", "true");
                mosesParams.put("report-all-factors", "true");
                // The XmlRpcClient.execute method doesn't accept Hashmap (pParams). It's either Object[] or List. 
                Object[] params = new Object[] { null };
                params[0] = mosesParams;
                // Invoke the remote method "translate". The result is an Object, convert it to a HashMap.
                HashMap result;
                try {
                    result = (HashMap)client.execute("translate", params);
                } catch (XmlRpcException ex) {
                    Logger.getLogger(Translate.class.getName()).log(Level.SEVERE, null, ex);
                    throw new IOException("XML-RPC failed");
                }
                // Print the returned results
                String textTranslation = (String)result.get("text");
                System.out.println("Input : "+textToTranslate);
                System.out.println("Translation : "+textTranslation);
                out.write(textTranslation);
                if (result.get("align") != null){ 
                        Object[] aligns = (Object[])result.get("align");
                        System.out.println("Phrase alignments : [Source Start:Source End][Target Start]"); 
                        for ( Object element : aligns) {
                                HashMap align = (HashMap)element;	
                                System.out.println("["+align.get("src-start")+":"+align.get("src-end")+"]["+align.get("tgt-start")+"]");
                        }
                }				
        } finally {            
            out.close();
        }
    }

    // <editor-fold defaultstate="collapsed" desc="HttpServlet methods. Click on the + sign on the left to edit the code.">
    /**
     * Handles the HTTP
     * <code>GET</code> method.
     *
     * @param request servlet request
     * @param response servlet response
     * @throws ServletException if a servlet-specific error occurs
     * @throws IOException if an I/O error occurs
     */
    @Override
    protected void doGet(HttpServletRequest request, HttpServletResponse response)
            throws ServletException, IOException {
        processRequest(request, response);
    }

    /**
     * Handles the HTTP
     * <code>POST</code> method.
     *
     * @param request servlet request
     * @param response servlet response
     * @throws ServletException if a servlet-specific error occurs
     * @throws IOException if an I/O error occurs
     */
    @Override
    protected void doPost(HttpServletRequest request, HttpServletResponse response)
            throws ServletException, IOException {
        processRequest(request, response);
    }

    /**
     * Returns a short description of the servlet.
     *
     * @return a String containing servlet description
     */
    @Override
    public String getServletInfo() {
        return "Short description";
    }// </editor-fold>
}
