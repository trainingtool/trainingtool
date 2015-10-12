package com.stac.powertrainer_44;

/**
 * Created by Art on 10/8/2015.
 */
public class D {
    public static void p(String str) {
        System.out.println(str);
    }
    public static int tryParseInt(String str, int def)
    {
        try
        {
            return Integer.parseInt(str);
        }
        catch(Exception e)
        {
            D.p("Could not parse integer '" + str + "'");
            return def;
        }
    }
    public static int tryParseInt(String str)
    {
        return tryParseInt(str, 0);
    }
    public static float tryParseFloat(String str)
    {
        try
        {
            return Float.parseFloat(str);
        }
        catch(Exception e)
        {
            D.p("Could not parse float '" + str + "'");
            return 0;
        }
    }
}
